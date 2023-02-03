
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <boards.h>
#include <onex-kernel/boot.h>
#if defined(DO_LATER)
#include <onex-kernel/log.h>
#endif
#include <onex-kernel/time.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/i2c.h>
#include <onex-kernel/spi.h>
#include <onex-kernel/display.h>
#include <onex-kernel/gfx.h>
#include <onex-kernel/touch.h>
#if defined(DO_LATER)
#include <onex-kernel/motion.h>
#endif
#include <onn.h>
#include <onr.h>

#include "g2d.h"

static object* user;
#if defined(DO_LATER)
static object* battery;
#endif
static object* touch;
#if defined(DO_LATER)
static object* motion;
#endif
static object* button;
static object* backlight;
static object* oclock;
static object* watchface;
static object* viewlist;
static object* home;
static object* notes;
static object* about;

static char* deviceuid;
static char* useruid;
#if defined(DO_LATER)
static char* batteryuid;
#endif
static char* touchuid;
#if defined(DO_LATER)
static char* motionuid;
#endif
static char* buttonuid;
static char* backlightuid;
static char* clockuid;
static char* watchfaceuid;
static char* viewlistuid;
static char* homeuid;
static char* notesuid;
static char* aboutuid;

static volatile bool          button_pressed=false;
static volatile touch_info_t  touch_info={ 120, 140 };
static volatile bool          new_touch_info=false;
#if defined(DO_LATER)
static volatile uint16_t      touch_info_stroke=0;
static volatile motion_info_t motion_info;
#endif

static void every_second(){
  onex_run_evaluators(clockuid, 0);
  onex_run_evaluators(aboutuid, 0);
}

static void every_10s(){
#if defined(DO_LATER)
  onex_run_evaluators(batteryuid, 0);
#endif
}

static bool user_active=true;

static void touched(touch_info_t ti)
{
  touch_info=ti;
  new_touch_info=true;

#if defined(DO_LATER)
  // XXX all in main loop not here
  static uint16_t swipe_start_x=0;
  static uint16_t swipe_start_y=0;

  if(touch_info.action==TOUCH_ACTION_CONTACT){
    swipe_start_x=touch_info.x;
    swipe_start_y=touch_info.y;
    touch_info_stroke=0;
  }
  else
  if(touch_info.action!=TOUCH_ACTION_CONTACT){
    int16_t dx=touch_info.x-swipe_start_x;
    int16_t dy=touch_info.y-swipe_start_y;
    touch_info_stroke=(uint16_t)sqrtf(dx*dx+dy*dy);
  }
#endif

  onex_run_evaluators(touchuid, 0);

#if defined(DO_LATER)
  // XXX all in main loop not here
  static uint8_t  disable_user_touch=0;

  if(!user_active          && touch_info.action==TOUCH_ACTION_CONTACT) disable_user_touch=1;
  if(disable_user_touch==1 && touch_info.action!=TOUCH_ACTION_CONTACT) disable_user_touch=2;
  if(disable_user_touch==2 && touch_info.action==TOUCH_ACTION_CONTACT) disable_user_touch=0;

  if(!disable_user_touch){
#if defined(DO_LATER)
    log_write("eval user from touched\n");
#endif
    onex_run_evaluators(useruid, 0);
  }
#endif
}

#if defined(DO_LATER)
static void moved(motion_info_t mi)
{
  motion_info=mi;
  onex_run_evaluators(motionuid, 0);
}
#endif

#define BUTTON_ACTION_NONE  0
#define BUTTON_ACTION_WAIT  1
#define BUTTON_ACTION_SHORT 2
#define BUTTON_ACTION_LONG  3
static uint8_t button_action = BUTTON_ACTION_NONE;

static void button_changed(uint8_t pin, uint8_t type){

  button_pressed = (gpio_get(BUTTON_1)==BUTTONS_ACTIVE_STATE);
  onex_run_evaluators(buttonuid, 0);
}

#if defined(DO_LATER)
static void charging_changed(uint8_t pin, uint8_t type){
  onex_run_evaluators(batteryuid, 0);
}
#endif

static uint8_t fps = 111;

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

void key_hit(unsigned char ch, uint8_t command){

  if(ch){
    add_char(ch);
    return;
  }
  if(command==G2D_KEYBOARD_COMMAND_DELETE){
    del_char();
    return;
  }
}

static void set_up_gpio(void)
{
  gpio_mode_cb(BUTTON_1, INPUT_PULLDOWN, RISING_AND_FALLING, button_changed);
#if defined(DO_LATER)
  gpio_mode_cb(CHARGE_SENSE, INPUT, RISING_AND_FALLING, charging_changed);
  gpio_adc_init(BATTERY_V, ADC_CHANNEL);
#endif
  gpio_mode_cb(BUTTON_1, INPUT_PULLDOWN, RISING_AND_FALLING, button_changed);
  gpio_mode(I2C_ENABLE, OUTPUT);
  gpio_set( I2C_ENABLE, 1);
  gpio_mode(LCD_BACKLIGHT, OUTPUT);
  gpio_set(LCD_BACKLIGHT, LEDS_ACTIVE_STATE);
}

static bool evaluate_default(object* o, void* d);
static bool evaluate_user(object* o, void* d);
#if defined(DO_LATER)
static bool evaluate_battery_in(object* o, void* d);
#endif
static bool evaluate_touch_in(object* o, void* d);
#if defined(DO_LATER)
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

int main()
{
  boot_init();
#if defined(DO_LATER)
  log_init();
#endif
  time_init();
  gpio_init();

  set_up_gpio();

  touch_init(touched);
#if defined(DO_LATER)
  motion_init(moved);
#endif

  g2d_init();

  onex_init("");

  onex_set_evaluators("default",   evaluate_default, 0);
  onex_set_evaluators("device",    evaluate_device_logic, 0);
  onex_set_evaluators("user",      evaluate_user, 0);
#if defined(DO_LATER)
  onex_set_evaluators("battery",   evaluate_battery_in, 0);
#endif
  onex_set_evaluators("touch",     evaluate_touch_in, 0);
#if defined(DO_LATER)
  onex_set_evaluators("motion",    evaluate_motion_in, 0);
#endif
  onex_set_evaluators("button",    evaluate_button_in, 0);
  onex_set_evaluators("about",     evaluate_about_in, 0);
  onex_set_evaluators("backlight", evaluate_edit_rule, evaluate_light_logic, evaluate_backlight_out, 0);
  onex_set_evaluators("clock",     evaluate_clock_sync, evaluate_clock, 0);
  onex_set_evaluators("editable",  evaluate_edit_rule, 0);

  object_set_evaluator(onex_device_object, "device");

  user     =object_new(0, "user",      "user", 8);
#if defined(DO_LATER)
  battery  =object_new(0, "battery",   "battery", 4);
#endif
  touch    =object_new(0, "touch",     "touch", 6);
#if defined(DO_LATER)
  motion   =object_new(0, "motion",    "motion", 8);
#endif
  button   =object_new(0, "button",    "button", 4);
  backlight=object_new(0, "backlight", "light editable", 9);
  oclock   =object_new(0, "clock",     "clock event", 12);
  watchface=object_new(0, "editable",  "watchface editable", 6);
  viewlist =object_new(0, "editable",  "list editable", 4);
  home     =object_new(0, "default",   "home", 4);
  notes    =object_new(0, "editable",  "note list editable", 4);
  about    =object_new(0, "about",     "about", 4);

  deviceuid   =object_property(onex_device_object, "UID");
  useruid     =object_property(user, "UID");
#if defined(DO_LATER)
  batteryuid  =object_property(battery, "UID");
#endif
  touchuid    =object_property(touch, "UID");
#if defined(DO_LATER)
  motionuid   =object_property(motion, "UID");
#endif
  buttonuid   =object_property(button, "UID");
  backlightuid=object_property(backlight, "UID");
  clockuid    =object_property(oclock, "UID");
  watchfaceuid=object_property(watchface, "UID");
  viewlistuid =object_property(viewlist, "UID");
  homeuid     =object_property(home, "UID");
  notesuid    =object_property(notes, "UID");
  aboutuid    =object_property(about, "UID");

  object_property_set(backlight, "light", "on");
  object_property_set(backlight, "level", "high");
  object_property_set(backlight, "timeout", "60000");
  object_property_set(backlight, "touch", touchuid);
#if defined(DO_LATER)
  object_property_set(backlight, "motion", motionuid);
#endif
  object_property_set(backlight, "button", buttonuid);

  object_property_set(oclock, "title", "OnexOS Clock");
  object_property_set(oclock, "ts", "%unknown");
  object_property_set(oclock, "tz", "%unknown");
  object_property_set(oclock, "device", deviceuid);

  object_property_set(watchface, "clock", clockuid);
  object_property_set(watchface, "ampm-24hr", "ampm");

  object_property_add(viewlist, (char*)"list", homeuid);
  object_property_add(viewlist, (char*)"list", notesuid);
  object_property_add(viewlist, (char*)"list", aboutuid);

#if defined(DO_LATER)
  object_property_set(home, (char*)"battery",   batteryuid);
#endif
  object_property_set(home, (char*)"watchface", watchfaceuid);

  object_property_set(user, "viewing", viewlistuid);

  object_property_add(onex_device_object, (char*)"user", useruid);
#if defined(DO_LATER)
  object_property_add(onex_device_object, (char*)"io",   batteryuid);
#endif
  object_property_add(onex_device_object, (char*)"io",   touchuid);
#if defined(DO_LATER)
  object_property_add(onex_device_object, (char*)"io",   motionuid);
#endif
  object_property_add(onex_device_object, (char*)"io",   buttonuid);
  object_property_add(onex_device_object, (char*)"io",   backlightuid);
  object_property_add(onex_device_object, (char*)"io",   clockuid);

  onex_run_evaluators(useruid, 0);
#if defined(DO_LATER)
  onex_run_evaluators(batteryuid, 0);
#endif
  onex_run_evaluators(clockuid, 0);
  onex_run_evaluators(backlightuid, 0);
  onex_run_evaluators(aboutuid, 0);

  time_ticker(every_second,  1000);
  time_ticker(every_10s,    10000);

  while(1){

    uint64_t ct=time_ms(); // XXX every loop?!

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

    // --------------------

#if defined(LOG_TO_GFX)
    if(event_log_buffer){
      draw_log();
      event_log_buffer=0;
    }
#endif

    // --------------------

    static uint64_t feeding_time=0;
    if(ct>feeding_time && !button_pressed){
      boot_feed_watchdog();
      feeding_time=ct+1000;
    }

    // --------------------

    // XXX put this in g2d to count actual frames shifted/s
    static uint8_t  frame_count = 0;
    static uint64_t tm_last = 0;

    frame_count++;
    if(ct > tm_last + 1000) {
      tm_last = ct;
      fps = frame_count;
      frame_count = 0;
    }

    // --------------------

    static uint64_t pressed_ts=0;
    if(button_pressed && !pressed_ts){
      button_action = BUTTON_ACTION_WAIT;
      pressed_ts=ct;
    }
    else
    if(button_pressed && pressed_ts){
      if(button_action == BUTTON_ACTION_WAIT){
        if((ct - pressed_ts) > 300){
          button_action = BUTTON_ACTION_LONG;
          onex_run_evaluators(useruid, 0);
        }
      }
    }
    else
    if(pressed_ts && !button_pressed){
      if(button_action == BUTTON_ACTION_WAIT){
        button_action = BUTTON_ACTION_SHORT;
        onex_run_evaluators(useruid, 0);
      }
      pressed_ts=0;
    }

    // --------------------

    if(new_touch_info){
      new_touch_info=false;

      static bool is_touched=false;

      bool is_down = touch_info.action==TOUCH_ACTION_DOWN ||
                     touch_info.action==TOUCH_ACTION_CONTACT;

      if(!is_touched && is_down){
        is_touched=true;
        g2d_touch_event(touch_info.x, touch_info.y);
        onex_run_evaluators(useruid, 0);
      }
      else
      if(is_touched && !is_down){
        is_touched=false;
      }
    }
  }
}

// ------------------- evaluators ----------------

// XXX separated out to remind me to extend the respective APIs to allow sprintf-style
// XXX varargs for path segments, value construction and g2d_text()
static char pathbuf[64];
static char valuebuf[64];
static char g2dbuf[64];

bool evaluate_default(object* o, void* d)
{
#if defined(DO_LATER)
  log_write("evaluate_default data=%p\n", d); object_log(o);
#endif
  return true;
}

#if defined(DO_LATER)
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
  snprintf(valuebuf, 64, "%d%%(%ld)", pc, mv);

  object_property_set(battery, "percent", valuebuf);

  int batt=gpio_get(CHARGE_SENSE);
  snprintf(valuebuf, 64, "%s", batt? "powering": "charging");
  object_property_set(battery, "status", valuebuf);

  return true;
}
#endif

bool evaluate_touch_in(object* o, void* d)
{
  snprintf(valuebuf, 64, "%3d %3d", touch_info.x, touch_info.y);
  object_property_set(touch, "coords", valuebuf);

  snprintf(valuebuf, 64, "%s", touch_actions[touch_info.action]);
  object_property_set(touch, "action", valuebuf);

#if defined(DO_LATER)
  snprintf(valuebuf, 64, "%d", touch_info_stroke);
  object_property_set(touch, "stroke", valuebuf);
#endif

  return true;
}

#if defined(DO_LATER)
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

  snprintf(valuebuf, 64, "%d %d %d %d", motion_info.x, motion_info.y, motion_info.z, motion_info.m);
  object_property_set(motion, "x-y-z-m", valuebuf);
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
  snprintf(valuebuf, 64, "%lu %lu", (unsigned long)&__BUILD_TIMESTAMP, (unsigned long)&__BOOTLOADER_NUMBER);
  object_property_set(about, "build-info", valuebuf);

  snprintf(valuebuf, 64, "%d%%", boot_cpu());
  object_property_set(about, "cpu", valuebuf);

  return true;
}

bool evaluate_backlight_out(object* o, void* d)
{
  bool light_on=object_property_is(backlight, "light", "on");

  if(light_on && !user_active){

#if defined(DO_LATER)
    display_wake();
#endif

    bool mid =object_property_is(backlight, "level", "mid");
    bool high=object_property_is(backlight, "level", "high");
    gpio_set(LCD_BACKLIGHT,      (mid||high)? LEDS_ACTIVE_STATE: !LEDS_ACTIVE_STATE);

#if defined(DO_LATER)
    touch_wake();
#endif

    user_active=true;

    onex_run_evaluators(useruid, 0);
  }
  else
  if(!light_on && user_active){

    user_active=false;

#if defined(DO_LATER)
  //touch_sleep();
#endif

    gpio_set(LCD_BACKLIGHT,      !LEDS_ACTIVE_STATE);

#if defined(DO_LATER)
    display_sleep();
#endif
  }
  return true;
}

// -------------------- User --------------------------

static void draw_by_type(char* path);
static void draw_home(char* path);
static void draw_notes(char* path);
static void draw_about(char* path);
static void draw_list(char* p);
static void draw_default(char* path);

static uint8_t list_index=1;

bool evaluate_user(object* o, void* d)
{
  if(!user_active) return true;

  if(button_action==BUTTON_ACTION_SHORT){
    list_index++;
    button_action=BUTTON_ACTION_NONE;
  }
  else
  if(button_action==BUTTON_ACTION_LONG){
    list_index--;
    button_action=BUTTON_ACTION_NONE;
  }

  draw_by_type("viewing");

  return true;
}

static char pi[64]; // XXX replace by pathbuf once recursion removed
static char pl[64];

void draw_by_type(char* p)
{
  snprintf(pi, 32, "%s:is", p);

  if(object_property_contains(user, pi, "home"))  draw_home(p);    else
  if(object_property_contains(user, pi, "note") &&
     object_property_contains(user, pi, "list") ) draw_notes(p);   else
  if(object_property_contains(user, pi, "about")) draw_about(p);   else
  if(object_property_contains(user, pi, "list"))  draw_list(p);    else
                                                  draw_default(p);
}

void draw_list(char* p)
{
  uint8_t list_len=object_property_length(user, "viewing:list");
  if(list_index<1       ) list_index=list_len;
  if(list_index>list_len) list_index=1;

  snprintf(pl, 32, "%s:list:%d", p, list_index); // XXX all goes wrong if this recurses (list:list) !

  draw_by_type(pl);
}

#define BATTERY_LOW      0x0 // RED
#define BATTERY_MED      0x0 // ORANGE
#define BATTERY_HIGH     0x0 // GREEN
#define BATTERY_CHARGING 0x0 // WHITE

void draw_home(char* path)
{
  snprintf(pathbuf, 64, "%s:battery:percent", path);      char* pc=object_property(   user, pathbuf);
  snprintf(pathbuf, 64, "%s:battery:status", path);       bool  ch=object_property_is(user, pathbuf, "charging");
  snprintf(pathbuf, 64, "%s:watchface:clock:ts", path);   char* ts=object_property(   user, pathbuf);
  snprintf(pathbuf, 64, "%s:watchface:clock:tz:2", path); char* tz=object_property(   user, pathbuf);
  snprintf(pathbuf, 64, "%s:watchface:ampm-24hr", path);  bool h24=object_property_is(user, pathbuf, "24hr");

  if(!ts) return;

  char* e;

  uint64_t tsnum=strtoull(ts,&e,10);
  if(*e) return;

  uint32_t tznum=strtoul(tz?tz:"0",&e,10);
  if(*e) tznum=0;

  time_t est=(time_t)(tsnum+tznum);
  struct tm tms={0};
  localtime_r(&est, &tms);

  g2d_clear_screen(0xff);

  strftime(g2dbuf, 64, h24? "%H:%M": "%l:%M", &tms);
  g2d_text(10, 90, g2dbuf, G2D_BLUE, G2D_WHITE, 7);

  strftime(g2dbuf, 64, h24? "24 %a %d %h": "%p %a %d %h", &tms);
  g2d_text(10, 170, g2dbuf, G2D_BLUE, G2D_WHITE, 3);

  g2d_render();

  int8_t pcnum=pc? (int8_t)strtol(pc,&e,10): 0;
  if(pcnum<0) pcnum=0;
  if(pcnum>100) pcnum=100;

  uint16_t batt_col;
  if(ch)       batt_col=BATTERY_CHARGING; else
  if(pcnum>67) batt_col=BATTERY_HIGH;     else
  if(pcnum>33) batt_col=BATTERY_MED;
  else         batt_col=BATTERY_LOW;
}

void draw_notes(char* path) {

  g2d_clear_screen(0xff);

  snprintf(g2dbuf, 64, "fps: %02d (%d,%d)", fps, touch_info.x, touch_info.y);
  g2d_text(10, 20, g2dbuf, G2D_BLUE, G2D_WHITE, 2);

  snprintf(g2dbuf, 64, "%s|", typed);
  g2d_text(10, 40, g2dbuf, G2D_BLUE, G2D_WHITE, 2);

  g2d_keyboard(key_hit);

  g2d_render();
}

void draw_about(char* path) {

  snprintf(pathbuf, 64, "%s:build-info", path); char* bnf=object_property_values(user, pathbuf);
  snprintf(pathbuf, 64, "%s:cpu", path);        char* cpu=object_property(       user, pathbuf);

  g2d_clear_screen(0xff);

  snprintf(g2dbuf, 64, "fps: %02d (%d,%d)", fps, touch_info.x, touch_info.y);
  g2d_text(10, 20, g2dbuf, G2D_BLUE, G2D_WHITE, 2);

  snprintf(g2dbuf, 64, "cpu: %s", cpu);
  g2d_text(10, 70, g2dbuf, G2D_BLUE, G2D_WHITE, 3);

  snprintf(g2dbuf, 64, "build: %s", bnf);
  g2d_text(10, 190, g2dbuf, G2D_BLUE, G2D_WHITE, 1);

  g2d_render();
}

void draw_default(char* path)
{
  snprintf(pathbuf, 64, "%s:is", path); char* is=object_property(user, pathbuf);
  // set_text(is_label, is);
}

#if defined(LOG_TO_GFX)
void draw_log()
{
  // set_text(log_label, (const char*)event_log_buffer);
}
#endif

