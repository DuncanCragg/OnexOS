
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <boards.h>
#include <onex-kernel/boot.h>
#if defined(BOARD_PINETIME)
#include <onex-kernel/log.h>
#endif
#include <onex-kernel/time.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/i2c.h>
#include <onex-kernel/spi.h>
#if defined(BOARD_PINETIME)
#include <onex-kernel/blenus.h>
#endif
#include <onex-kernel/display.h>
#include <onex-kernel/gfx.h>
#include <onex-kernel/touch.h>
#if defined(BOARD_PINETIME)
#include <onex-kernel/motion.h>
#endif
#include <onn.h>
#include <onr.h>

static object* user;
#if defined(BOARD_PINETIME)
static object* battery;
static object* bluetooth;
#endif
static object* touch;
#if defined(BOARD_PINETIME)
static object* motion;
#endif
static object* button;
static object* backlight;
static object* oclock;
static object* watchface;
static object* viewlist;
static object* home;
static object* calendar;
static object* about;

static char* deviceuid;
static char* useruid;
#if defined(BOARD_PINETIME)
static char* batteryuid;
static char* bluetoothuid;
#endif
static char* touchuid;
#if defined(BOARD_PINETIME)
static char* motionuid;
#endif
static char* buttonuid;
static char* backlightuid;
static char* clockuid;
static char* watchfaceuid;
static char* viewlistuid;
static char* homeuid;
static char* calendaruid;
static char* aboutuid;

static volatile touch_info_t  touch_info={ 120, 140 };
static volatile bool          new_touch_info=false;
static volatile uint16_t      touch_info_stroke=0;
#if defined(BOARD_PINETIME)
static volatile motion_info_t motion_info;
static volatile blenus_info_t ble_info={ .connected=false, .rssi=-100 };
#endif

static char buf[64];

static void every_second(){
  onex_run_evaluators(clockuid, 0);
  onex_run_evaluators(aboutuid, 0);
}

static void every_10s(){
#if defined(BOARD_PINETIME)
  onex_run_evaluators(batteryuid, 0);
#endif
}

#if defined(BOARD_PINETIME)
static void blechanged(blenus_info_t bi)
{
  ble_info=bi;
  onex_run_evaluators(bluetoothuid, 0);
}
#endif

static bool user_active=true;

static void touched(touch_info_t ti)
{
  touch_info=ti;
  new_touch_info=true;

  static uint16_t swipe_start_x=0;
  static uint16_t swipe_start_y=0;

  bool is_gesture=(touch_info.gesture==TOUCH_GESTURE_LEFT  ||
                   touch_info.gesture==TOUCH_GESTURE_RIGHT ||
                   touch_info.gesture==TOUCH_GESTURE_DOWN  ||
                   touch_info.gesture==TOUCH_GESTURE_UP      );

  if(touch_info.action==TOUCH_ACTION_CONTACT && !is_gesture){
    swipe_start_x=touch_info.x;
    swipe_start_y=touch_info.y;
    touch_info_stroke=0;
  }
  else
  if(touch_info.action!=TOUCH_ACTION_CONTACT && is_gesture){
    int16_t dx=touch_info.x-swipe_start_x;
    int16_t dy=touch_info.y-swipe_start_y;
    touch_info_stroke=(uint16_t)sqrtf(dx*dx+dy*dy);
  }

  onex_run_evaluators(touchuid, 0);


  static uint8_t  disable_user_touch=0;

  if(!user_active          && touch_info.action==TOUCH_ACTION_CONTACT) disable_user_touch=1;
  if(disable_user_touch==1 && touch_info.action!=TOUCH_ACTION_CONTACT) disable_user_touch=2;
  if(disable_user_touch==2 && touch_info.action==TOUCH_ACTION_CONTACT) disable_user_touch=0;

  if(!disable_user_touch){
#if defined(BOARD_PINETIME)
    log_write("eval user from touched %d\n", is_gesture);
#endif
    onex_run_evaluators(useruid, (void*)1);
  }
}

#if defined(BOARD_PINETIME)
static void moved(motion_info_t mi)
{
  motion_info=mi;
  onex_run_evaluators(motionuid, 0);
}
#endif

static void button_changed(uint8_t pin, uint8_t type){
  onex_run_evaluators(buttonuid, 0);
}

#if defined(BOARD_PINETIME)
static void charging_changed(uint8_t pin, uint8_t type){
  onex_run_evaluators(batteryuid, 0);
}
#endif

static void set_up_gpio(void)
{
  gpio_mode_cb(BUTTON_1, INPUT_PULLDOWN, RISING_AND_FALLING, button_changed);
#if defined(BOARD_PINETIME)
  gpio_mode(   BUTTON_ENABLE, OUTPUT);
  gpio_set(    BUTTON_ENABLE, 1);
  gpio_mode_cb(CHARGE_SENSE, INPUT, RISING_AND_FALLING, charging_changed);
  gpio_adc_init(BATTERY_V, ADC_CHANNEL);

  gpio_mode(LCD_BACKLIGHT_LOW, OUTPUT);
  gpio_mode(LCD_BACKLIGHT_MID, OUTPUT);
  gpio_mode(LCD_BACKLIGHT_HIGH, OUTPUT);
#elif defined(BOARD_MAGIC3)
  gpio_mode_cb(BUTTON_1, INPUT_PULLDOWN, RISING_AND_FALLING, button_changed);
  gpio_mode(I2C_ENABLE, OUTPUT);
  gpio_set( I2C_ENABLE, 1);
  gpio_mode(LCD_BACKLIGHT, OUTPUT);
  gpio_set(LCD_BACKLIGHT, LEDS_ACTIVE_STATE);
#endif
}

static bool evaluate_default(object* o, void* d);
static bool evaluate_user(object* o, void* d);
#if defined(BOARD_PINETIME)
static bool evaluate_battery_in(object* o, void* d);
static bool evaluate_bluetooth_in(object* o, void* d);
#endif
static bool evaluate_touch_in(object* o, void* d);
#if defined(BOARD_PINETIME)
static bool evaluate_motion_in(object* o, void* d);
#endif
static bool evaluate_button_in(object* o, void* d);
static bool evaluate_about_in(object* o, void* d);
static bool evaluate_backlight_out(object* o, void* d);

#if defined(LOG_TO_GFX)
extern volatile char* event_log_buffer;
static void draw_log();
#endif

#define ADC_CHANNEL 0

static char    typed[64];
static uint8_t cursor=0;

void del_char()
{
  if(cursor==0) return;
  typed[--cursor]=0;
}

void add_char(char c)
{
  if(cursor==64) return;
  typed[cursor++]=c;
  typed[cursor]=0;
}

uint8_t key_index(uint16_t x, uint16_t y)
{
  uint8_t xstart=40;
  uint8_t xend  =170;
  uint8_t ystart=130;
  uint8_t xinc  =30;
  if(x<xstart) x=xstart;
  if(x>xend)   x=xend;
  if(y<ystart) y=ystart;
  if(y>=130 && y<=165){
    return 1+(x-xstart)/xinc;
  }
  else
  if(y>165 && y<=200){
    return 6+(x-xstart)/xinc;
  }
  else
  if(y>200 && y<=235){
    return 11+(x-xstart)/xinc;
  }
  else
  if(y>235 && y<=280){
    return 16+(x-xstart)/xinc;
  }
  return 0;
}

int main()
{
  boot_init();
#if defined(BOARD_PINETIME)
  log_init();
#endif
  time_init();
  gpio_init();
#if defined(BOARD_PINETIME)
  blenus_init(0, blechanged);
#endif

  set_up_gpio();

#if defined(BOARD_PINETIME)
  display_init();
#endif

  touch_init(touched);
#if defined(BOARD_PINETIME)
  motion_init(moved);
#endif

#if defined(BOARD_MAGIC3)
  gfx_fast_init();
#endif

  onex_init("");

  onex_set_evaluators("default",   evaluate_default, 0);
  onex_set_evaluators("device",    evaluate_device_logic, 0);
  onex_set_evaluators("user",      evaluate_user, 0);
#if defined(BOARD_PINETIME)
  onex_set_evaluators("battery",   evaluate_battery_in, 0);
  onex_set_evaluators("bluetooth", evaluate_bluetooth_in, 0);
#endif
  onex_set_evaluators("touch",     evaluate_touch_in, 0);
#if defined(BOARD_PINETIME)
  onex_set_evaluators("motion",    evaluate_motion_in, 0);
#endif
  onex_set_evaluators("button",    evaluate_button_in, 0);
  onex_set_evaluators("about",     evaluate_about_in, 0);
  onex_set_evaluators("backlight", evaluate_edit_rule, evaluate_light_logic, evaluate_backlight_out, 0);
  onex_set_evaluators("clock",     evaluate_clock_sync, evaluate_clock, 0);
  onex_set_evaluators("editable",  evaluate_edit_rule, 0);

  object_set_evaluator(onex_device_object, "device");

  user     =object_new(0, "user",      "user", 8);
#if defined(BOARD_PINETIME)
  battery  =object_new(0, "battery",   "battery", 4);
  bluetooth=object_new(0, "bluetooth", "bluetooth", 4);
#endif
  touch    =object_new(0, "touch",     "touch", 6);
#if defined(BOARD_PINETIME)
  motion   =object_new(0, "motion",    "motion", 8);
#endif
  button   =object_new(0, "button",    "button", 4);
  backlight=object_new(0, "backlight", "light editable", 9);
  oclock   =object_new(0, "clock",     "clock event", 12);
  watchface=object_new(0, "editable",  "watchface editable", 6);
  viewlist =object_new(0, "editable",  "list editable", 4);
  home     =object_new(0, "default",   "home", 4);
  calendar =object_new(0, "editable",  "event list editable", 4);
  about    =object_new(0, "about",     "about", 4);

  deviceuid   =object_property(onex_device_object, "UID");
  useruid     =object_property(user, "UID");
#if defined(BOARD_PINETIME)
  batteryuid  =object_property(battery, "UID");
  bluetoothuid=object_property(bluetooth, "UID");
#endif
  touchuid    =object_property(touch, "UID");
#if defined(BOARD_PINETIME)
  motionuid   =object_property(motion, "UID");
#endif
  buttonuid   =object_property(button, "UID");
  backlightuid=object_property(backlight, "UID");
  clockuid    =object_property(oclock, "UID");
  watchfaceuid=object_property(watchface, "UID");
  viewlistuid =object_property(viewlist, "UID");
  homeuid     =object_property(home, "UID");
  calendaruid =object_property(calendar, "UID");
  aboutuid    =object_property(about, "UID");

  object_property_set(backlight, "light", "on");
  object_property_set(backlight, "level", "high");
  object_property_set(backlight, "timeout", "60000");
  object_property_set(backlight, "touch", touchuid);
#if defined(BOARD_PINETIME)
  object_property_set(backlight, "motion", motionuid);
#endif
  object_property_set(backlight, "button", buttonuid);

  object_property_set(oclock, "title", "OnexOS Clock");
  object_property_set(oclock, "ts", "%unknown");
  object_property_set(oclock, "tz", "%unknown");
  object_property_set(oclock, "device", deviceuid);

  object_property_set(watchface, "clock", clockuid);
  object_property_set(watchface, "ampm-24hr", "ampm");

  object_property_add(viewlist, (char*)"list", calendaruid);
  object_property_add(viewlist, (char*)"list", aboutuid);
  object_property_add(viewlist, (char*)"list", homeuid);

#if defined(BOARD_PINETIME)
  object_property_set(home, (char*)"battery",   batteryuid);
  object_property_set(home, (char*)"bluetooth", bluetoothuid);
#endif
  object_property_set(home, (char*)"watchface", watchfaceuid);

  object_property_set(user, "viewing", viewlistuid);

  object_property_add(onex_device_object, (char*)"user", useruid);
#if defined(BOARD_PINETIME)
  object_property_add(onex_device_object, (char*)"io",   batteryuid);
  object_property_add(onex_device_object, (char*)"io",   bluetoothuid);
#endif
  object_property_add(onex_device_object, (char*)"io",   touchuid);
#if defined(BOARD_PINETIME)
  object_property_add(onex_device_object, (char*)"io",   motionuid);
#endif
  object_property_add(onex_device_object, (char*)"io",   buttonuid);
  object_property_add(onex_device_object, (char*)"io",   backlightuid);
  object_property_add(onex_device_object, (char*)"io",   clockuid);

  onex_run_evaluators(useruid, 0);
#if defined(BOARD_PINETIME)
  onex_run_evaluators(batteryuid, 0);
  onex_run_evaluators(bluetoothuid, 0);
#endif
  onex_run_evaluators(clockuid, 0);
  onex_run_evaluators(backlightuid, 0);
  onex_run_evaluators(aboutuid, 0);

  time_ticker(every_second,  1000);
  time_ticker(every_10s,    10000);

  while(1){

    uint64_t ct=time_ms();

#if defined(LOG_LOOP_TIME)
    static uint64_t lt=0;
    if(lt) log_write("loop time %ldms", (uint32_t)(ct-lt));
    lt=ct;
#endif

    if(!onex_loop()){
      gpio_sleep(); // will gpio_wake() when ADC read
      spi_sleep();  // will spi_wake() as soon as spi_tx called
      i2c_sleep();  // will i2c_wake() in irq to read values
      boot_sleep();
    }

#if defined(LOG_TO_GFX)
    if(event_log_buffer){
      draw_log();
      event_log_buffer=0;
    }
#endif

    static uint64_t feeding_time=0;
    if(ct>feeding_time && gpio_get(BUTTON_1)!=BUTTONS_ACTIVE_STATE){
      boot_feed_watchdog();
      feeding_time=ct+1000;
    }

    static uint8_t  frame_count = 0;
    static uint64_t tm_last = 0;
    static uint8_t  fps = 111;

    frame_count++;
    uint64_t tm=time_ms();
    if(tm > tm_last + 1000) {
      tm_last = tm;
      fps = frame_count;
      frame_count = 0;
    }

    gfx_fast_clear_screen(0xff);

    static char buf[64];

    snprintf(buf, 64, "fps: %02d (%d,%d)", fps, touch_info.x, touch_info.y);
    gfx_fast_text(10, 20, buf, 0x001a, 0xffff, 2);

    snprintf(buf, 64, "%s|", typed);
    gfx_fast_text(10, 40, buf, 0x001a, 0xffff, 2);

    uint8_t kbdstart_x=15;
    uint8_t kbdstart_y=115;
    uint8_t rowspacing=40;

    static uint8_t kbpg=1;

    static char key_indexes_page[7][21]={
                        { "      " },
                        { " "
                          "ERTIO"
                          "ASDGH"
                          "CBNM>"
                          "^#  ~" },
                        { " "
                          "QWYUP"
                          "FJKL "
                          " ZXV<"
                          "^#  ~" },
                        { " "
                          "+ ()-"
                          "/'#@*"
                          "%!;:>"
                          "^#,.~" },
                        { " "
                          "^&[]_"
                          " \"{}~"
                          " ?<><"
                          "^#,.~" },
                        { " "
                          "+123-"
                          "/456*"
                          "%789>"
                          "^#0.=" },
                        { " "
                          "^123e"
                          " 456 "
                          " 789<"
                          "^#0.=" }};

    #define SELECT_PAGE 15
    #define SELECT_TYPE 16
    #define DELETE_LAST 17

    for(uint8_t j=0; j<4; j++){

      snprintf(buf, 64, "%c %c %c %c %c",
                         key_indexes_page[kbpg][1+j*5],
                         key_indexes_page[kbpg][2+j*5],
                         key_indexes_page[kbpg][3+j*5],
                         key_indexes_page[kbpg][4+j*5],
                         key_indexes_page[kbpg][5+j*5]);

      gfx_fast_text(kbdstart_x, kbdstart_y+rowspacing*(j), buf, 0x0000, 0xffff, 4);
    }

    gfx_fast_write_out_buffer();

    if(new_touch_info){
      new_touch_info=false;

      static bool is_touched=false;
      if(!is_touched && touch_info.action==TOUCH_ACTION_CONTACT){
        is_touched=true;

        uint8_t ki = key_index(touch_info.x, touch_info.y);
        if(ki==SELECT_TYPE){
          if(kbpg==1 || kbpg==2) kbpg=3;
          else
          if(kbpg==3 || kbpg==4) kbpg=5;
          else
          if(kbpg==5 || kbpg==6) kbpg=1;
        }
        else
        if(ki==SELECT_PAGE){
          if(kbpg==1) kbpg=2;
          else
          if(kbpg==2) kbpg=1;
          else
          if(kbpg==3) kbpg=4;
          else
          if(kbpg==4) kbpg=3;
          else
          if(kbpg==5) kbpg=6;
          else
          if(kbpg==6) kbpg=5;
        }
        else
        if(ki==DELETE_LAST){

          del_char();
        }
        else
        if(ki>=1){

          add_char(key_indexes_page[kbpg][ki]);

          if(kbpg==2 || kbpg==6) kbpg--;
          else
          if(kbpg==3 || kbpg==4) kbpg=1;
        }
        else
        if(ki==0) add_char('?');
      }
      else
      if(is_touched && touch_info.action!=TOUCH_ACTION_CONTACT){
        is_touched=false;
      }
    }

  }
}

// ------------------- evaluators ----------------

bool evaluate_default(object* o, void* d)
{
#if defined(BOARD_PINETIME)
  log_write("evaluate_default data=%p\n", d); object_log(o);
#endif
  return true;
}

#if defined(BOARD_PINETIME)
#define BATTERY_ZERO_PERCENT 3400
#define BATTERY_100_PERCENT 4000
#define BATTERY_PERCENT_STEPS 2
bool evaluate_battery_in(object* o, void* d)
{
  int32_t bv = gpio_read(ADC_CHANNEL);
  int32_t mv = bv*2000/(1024/(33/10));
  int8_t pc = ((mv-BATTERY_ZERO_PERCENT)*100/((BATTERY_100_PERCENT-BATTERY_ZERO_PERCENT)*BATTERY_PERCENT_STEPS))*BATTERY_PERCENT_STEPS;
  if(pc<0) pc=0;
  if(pc>100) pc=100;
  snprintf(buf, 16, "%d%%(%ld)", pc, mv);

  object_property_set(battery, "percent", buf);

  int batt=gpio_get(CHARGE_SENSE);
  snprintf(buf, 16, "%s", batt? "powering": "charging");
  object_property_set(battery, "status", buf);

  return true;
}
#endif

#if defined(BOARD_PINETIME)
bool evaluate_bluetooth_in(object* o, void* d)
{
  object_property_set(bluetooth, "connected", ble_info.connected? "yes": "no");
  snprintf(buf, 16, "%3d", ble_info.rssi);
  object_property_set(bluetooth, "rssi", buf);
  return true;
}
#endif

bool evaluate_touch_in(object* o, void* d)
{
  snprintf(buf, 64, "%3d %3d", touch_info.x, touch_info.y);
  object_property_set(touch, "coords", buf);

  snprintf(buf, 64, "%s %s", touch_actions[touch_info.action], touch_gestures[touch_info.gesture]);
  object_property_set(touch, "action", buf);

  snprintf(buf, 64, "%d", touch_info_stroke);
  object_property_set(touch, "stroke", buf);

  return true;
}

#if defined(BOARD_PINETIME)
bool evaluate_motion_in(object* o, void* d)
{
  static int16_t prevx=0;
  static int16_t prevm=0;
  bool viewscreen=(prevx < -300 &&
                   motion_info.x < -700 &&
                   motion_info.x > -1200 &&
                   abs(motion_info.y) < 600 &&
                   abs(prevm) > 70);
  prevx=motion_info.x;
  prevm=motion_info.m;

  static uint32_t ticks=0;
  ticks++;
  if(ticks%50 && !viewscreen) return true;

  snprintf(buf, 64, "%d %d %d %d", motion_info.x, motion_info.y, motion_info.z, motion_info.m);
  object_property_set(motion, "x-y-z-m", buf);
  object_property_set(motion, "gesture", viewscreen? "view-screen": "none");

  return true;
}
#endif

bool evaluate_button_in(object* o, void* d)
{
  bool button_pressed=(gpio_get(BUTTON_1)==BUTTONS_ACTIVE_STATE);
  object_property_set(button, "state", button_pressed? "down": "up");
  return true;
}

extern char __BUILD_TIMESTAMP;
extern char __BOOTLOADER_NUMBER;

bool evaluate_about_in(object* o, void* d)
{
  snprintf(buf, 32, "%lu %lu", (unsigned long)&__BUILD_TIMESTAMP, (unsigned long)&__BOOTLOADER_NUMBER);
  object_property_set(about, "build-info", buf);

  snprintf(buf, 16, "%d%%", boot_cpu());
  object_property_set(about, "cpu", buf);

  return true;
}

bool evaluate_backlight_out(object* o, void* d)
{
  bool light_on=object_property_is(backlight, "light", "on");

  if(light_on && !user_active){

#if defined(BOARD_PINETIME)
    display_wake();
#endif

    bool mid =object_property_is(backlight, "level", "mid");
    bool high=object_property_is(backlight, "level", "high");
#if defined(BOARD_PINETIME)
    gpio_set(LCD_BACKLIGHT_LOW,               LEDS_ACTIVE_STATE);
    gpio_set(LCD_BACKLIGHT_MID,  (mid||high)? LEDS_ACTIVE_STATE: !LEDS_ACTIVE_STATE);
    gpio_set(LCD_BACKLIGHT_HIGH, (high)?      LEDS_ACTIVE_STATE: !LEDS_ACTIVE_STATE);
#elif defined(BOARD_MAGIC3)
    gpio_set(LCD_BACKLIGHT,      (mid||high)? LEDS_ACTIVE_STATE: !LEDS_ACTIVE_STATE);
#endif

  //touch_wake();

    user_active=true;

    onex_run_evaluators(useruid, 0);
  }
  else
  if(!light_on && user_active){

    user_active=false;

  //touch_sleep();

#if defined(BOARD_PINETIME)
    gpio_set(LCD_BACKLIGHT_LOW,  !LEDS_ACTIVE_STATE);
    gpio_set(LCD_BACKLIGHT_MID,  !LEDS_ACTIVE_STATE);
    gpio_set(LCD_BACKLIGHT_HIGH, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_MAGIC3)
    gpio_set(LCD_BACKLIGHT,      !LEDS_ACTIVE_STATE);
#endif

#if defined(BOARD_PINETIME)
    display_sleep();
#endif
  }
  return true;
}

// -------------------- User --------------------------

static void draw_by_type(char* path, bool touchevent);
static void draw_home(char* path);
static void draw_calendar(char* path);
static void draw_event(char* path);
static void draw_about(char* path);
static void draw_list(char* p, bool touchevent);
static void draw_default(char* path);

bool evaluate_user(object* o, void* touchevent)
{
  if(user_active) draw_by_type("viewing", !!touchevent);
  return true;
}

static char pi[64];
static char pl[64];

void draw_by_type(char* p, bool touchevent)
{
  snprintf(pi, 32, "%s:is", p);

  if(object_property_contains(user, pi, "home"))  draw_home(p);               else
  if(object_property_contains(user, pi, "event") &&
     object_property_contains(user, pi, "list") ) draw_calendar(p);           else
  if(object_property_contains(user, pi, "event")) draw_event(p);              else
  if(object_property_contains(user, pi, "about")) draw_about(p);              else
  if(object_property_contains(user, pi, "list"))  draw_list(p, !!touchevent); else
                                                  draw_default(p);
}

static uint8_t list_index=1;

void draw_list(char* p, bool touchevent)
{
  if(touchevent){
    if(touch_info.gesture==TOUCH_GESTURE_LEFT  && touch_info_stroke > 50){
      list_index++;
    }
    else
    if(touch_info.gesture==TOUCH_GESTURE_RIGHT && touch_info_stroke > 50){
      list_index--;
    }
  }
  uint8_t list_len=object_property_length(user, "viewing:list");
  if(list_index<1       ) list_index=list_len;
  if(list_index>list_len) list_index=1;

  snprintf(pl, 32, "%s:list:%d", p, list_index); // all goes wrong if this recurses (list:list) !

  draw_by_type(pl, false);
}

#define BATTERY_LOW      0x0 // RED
#define BATTERY_MED      0x0 // ORANGE
#define BATTERY_HIGH     0x0 // GREEN
#define BATTERY_CHARGING 0x0 // WHITE

void draw_home(char* path)
{
  snprintf(buf, 64, "%s:battery:percent", path);      char* pc=object_property(   user, buf);
  snprintf(buf, 64, "%s:battery:status", path);       bool  ch=object_property_is(user, buf, "charging");
  snprintf(buf, 64, "%s:bluetooth:connected", path);  bool  bl=object_property_is(user, buf, "yes");
  snprintf(buf, 64, "%s:watchface:clock:ts", path);   char* ts=object_property(   user, buf);
  snprintf(buf, 64, "%s:watchface:clock:tz:2", path); char* tz=object_property(   user, buf);
  snprintf(buf, 64, "%s:watchface:ampm-24hr", path);  bool h24=object_property_is(user, buf, "24hr");

  if(!ts) return;

  char* e;

  uint64_t tsnum=strtoull(ts,&e,10);
  if(*e) return;

  uint32_t tznum=strtoul(tz?tz:"0",&e,10);
  if(*e) tznum=0;

  time_t est=(time_t)(tsnum+tznum);
  struct tm tms={0};
  localtime_r(&est, &tms);

  char t[32];

  strftime(t, 32, h24? "%H:%M": "%l:%M", &tms);
  // set_text(time_label, t);

  strftime(t, 32, h24? "24 %a %d %h": "%p %a %d %h", &tms);
  // set_text(date_label, t);

  int8_t pcnum=pc? (int8_t)strtol(pc,&e,10): 0;
  if(pcnum<0) pcnum=0;
  if(pcnum>100) pcnum=100;

  uint16_t batt_col;
  if(ch)       batt_col=BATTERY_CHARGING; else
  if(pcnum>67) batt_col=BATTERY_HIGH;     else
  if(pcnum>33) batt_col=BATTERY_MED;
  else         batt_col=BATTERY_LOW;
}

void draw_calendar(char* path)
{
  // keyboard();
}

void draw_event(char* path)
{
}

void draw_about(char* path)
{
  snprintf(buf, 64, "%s:build-info", path); char* bnf=object_property_values(user, buf);
  snprintf(buf, 64, "%s:cpu", path);        char* cpu=object_property(       user, buf);

  // set_text(about_title, "About device");

  // set_text(build_label, bnf);

  // set_text(cpu_label, cpu);
}

void draw_default(char* path)
{
  snprintf(buf, 64, "%s:is", path); char* is=object_property(user, buf);
  // set_text(is_label, is);
}

#if defined(LOG_TO_GFX)
void draw_log()
{
  // set_text(log_label, (const char*)event_log_buffer);
}
#endif

