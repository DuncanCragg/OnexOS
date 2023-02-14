
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
static object* battery;
static object* touch;
#if defined(DO_LATER)
static object* motion;
#endif
static object* button;
static object* backlight;
static object* oclock;
static object* watchface;
static object* home;
static object* watch;
static object* notes;
static object* about;

static char* deviceuid;
static char* useruid;
static char* batteryuid;
static char* touchuid;
#if defined(DO_LATER)
static char* motionuid;
#endif
static char* buttonuid;
static char* backlightuid;
static char* clockuid;
static char* watchfaceuid;
static char* homeuid;
static char* watchuid;
static char* notesuid;
static char* aboutuid;

#define LONG_PRESS_MS 250
static volatile bool          button_pressed=false;

static volatile touch_info_t  touch_info={ 120, 140 };
static volatile bool          touch_down=false;
static volatile bool          touch_pending=false;
#if defined(DO_LATER)
static volatile uint16_t      touch_info_stroke=0;
static volatile motion_info_t motion_info;
#endif

static void every_second(){
  onex_run_evaluators(clockuid, 0);
  onex_run_evaluators(aboutuid, 0);
}

static void every_10s(){
  onex_run_evaluators(batteryuid, 0);
}

static bool user_active=true;

static uint32_t touch_events=0;
static uint32_t touch_events_seen=0;

static void touched(touch_info_t ti) {

  touch_events++;

  // maybe need to drive touch chip differently
  // or put this logic into the touch api
  #define TOUCH_OFFSET_X -35
  #define TOUCH_SCALE_X  135/100
  #define TOUCH_OFFSET_Y 0
  #define TOUCH_SCALE_Y  95/100
  int16_t x=TOUCH_OFFSET_X+ti.x*TOUCH_SCALE_X;
  int16_t y=TOUCH_OFFSET_Y+ti.y*TOUCH_SCALE_Y;
  if(x<0) x=0;
  if(x>=ST7789_WIDTH) x=ST7789_WIDTH-1;
  if(y<0) y=0;
  if(y>=ST7789_HEIGHT) y=ST7789_HEIGHT-1;
  ti.x=(uint16_t)x;
  ti.y=(uint16_t)y;

  // ---------------------------------------
  // XXX move this "down state" logic to the touch API?

  bool is_down_event = ti.action==TOUCH_ACTION_DOWN ||
                       ti.action==TOUCH_ACTION_CONTACT;

  if(!touch_down && is_down_event){ if(touch_pending) return; touch_down=true; }
  else
  if(touch_down && !is_down_event){ touch_down=false; touch_pending=true; }

  if(is_down_event) touch_pending=true;

  touch_info=ti;

  onex_run_evaluators(touchuid, 0);
}

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

static void charging_changed(uint8_t pin, uint8_t type){
  onex_run_evaluators(batteryuid, 0);
}

// --------------------------------------------------------

#define ADC_CHANNEL 0

static void set_up_gpio(void)
{
  gpio_mode_cb(CHARGE_SENSE, INPUT, RISING_AND_FALLING, charging_changed);
  gpio_adc_init(BATTERY_V, ADC_CHANNEL);
  gpio_mode_cb(BUTTON_1, INPUT_PULLDOWN, RISING_AND_FALLING, button_changed);
  gpio_mode(I2C_ENABLE, OUTPUT);
  gpio_set( I2C_ENABLE, 1);
  gpio_mode(LCD_BACKLIGHT, OUTPUT);
  gpio_set(LCD_BACKLIGHT, LEDS_ACTIVE_STATE);
}

static bool evaluate_default(object* o, void* d);
static bool evaluate_user(object* o, void* d);
static bool evaluate_battery_in(object* o, void* d);
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

extern char __BUILD_TIMESTAMP;
extern char __BUILD_TIME;
extern char __BOOTLOADER_NUMBER;

static uint32_t loop_time=0;

int main()
{
  boot_init();
#if defined(DO_LATER)
  log_init();
#endif
  time_init_set((unsigned long)&__BUILD_TIMESTAMP);
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
  onex_set_evaluators("battery",   evaluate_battery_in, 0);
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
  battery  =object_new(0, "battery",   "battery", 4);
  touch    =object_new(0, "touch",     "touch", 6);
#if defined(DO_LATER)
  motion   =object_new(0, "motion",    "motion", 8);
#endif
  button   =object_new(0, "button",    "button", 4);
  backlight=object_new(0, "backlight", "light editable", 9);
  oclock   =object_new(0, "clock",     "clock event", 12);
  watchface=object_new(0, "editable",  "watchface editable", 6);
  home     =object_new(0, "editable",  "list editable", 4);
  watch    =object_new(0, "default",   "watch", 4);
  notes    =object_new(0, "editable",  "note editable", 4);
  about    =object_new(0, "about",     "about", 4);

  deviceuid   =object_property(onex_device_object, "UID");
  useruid     =object_property(user, "UID");
  batteryuid  =object_property(battery, "UID");
  touchuid    =object_property(touch, "UID");
#if defined(DO_LATER)
  motionuid   =object_property(motion, "UID");
#endif
  buttonuid   =object_property(button, "UID");
  backlightuid=object_property(backlight, "UID");
  clockuid    =object_property(oclock, "UID");
  watchfaceuid=object_property(watchface, "UID");
  homeuid     =object_property(home, "UID");
  watchuid    =object_property(watch, "UID");
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

  object_property_add(home, (char*)"list", notesuid);
  object_property_add(home, (char*)"list", watchuid);
  object_property_add(home, (char*)"list", aboutuid);
  object_property_add(home, (char*)"list", notesuid);
  object_property_add(home, (char*)"list", watchuid);
  object_property_add(home, (char*)"list", aboutuid);
  object_property_add(home, (char*)"list", notesuid);
  object_property_add(home, (char*)"list", watchuid);
  object_property_add(home, (char*)"list", aboutuid);

  object_property_set(watch, (char*)"battery",   batteryuid);
  object_property_set(watch, (char*)"watchface", watchfaceuid);

  object_property_set(user, "viewing", watchuid);

  object_property_add(onex_device_object, (char*)"user", useruid);
  object_property_add(onex_device_object, (char*)"io",   batteryuid);
  object_property_add(onex_device_object, (char*)"io",   touchuid);
#if defined(DO_LATER)
  object_property_add(onex_device_object, (char*)"io",   motionuid);
#endif
  object_property_add(onex_device_object, (char*)"io",   buttonuid);
  object_property_add(onex_device_object, (char*)"io",   backlightuid);
  object_property_add(onex_device_object, (char*)"io",   clockuid);

  onex_run_evaluators(useruid, 0);
  onex_run_evaluators(batteryuid, 0);
  onex_run_evaluators(clockuid, 0);
  onex_run_evaluators(backlightuid, 0);
  onex_run_evaluators(aboutuid, 0);

  time_ticker(every_second,  1000);
  time_ticker(every_10s,    10000);

  while(1){

    uint64_t ct=time_ms();
    static uint64_t lt=0;
    if(lt) loop_time=(uint32_t)(ct-lt);
    lt=ct;

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

    static uint64_t pressed_ts=0;
    if(button_pressed && !pressed_ts){
      button_action = BUTTON_ACTION_WAIT;
      pressed_ts=ct;
    }
    else
    if(button_pressed && pressed_ts){
      if(button_action == BUTTON_ACTION_WAIT){
        if((ct - pressed_ts) > LONG_PRESS_MS){
          button_action = BUTTON_ACTION_LONG;
          touch_down=false; // button seems to send touch events after reboot
          onex_run_evaluators(useruid, 0);
        }
      }
    }
    else
    if(pressed_ts && !button_pressed){
      if(button_action == BUTTON_ACTION_WAIT){
        button_action = BUTTON_ACTION_SHORT;
        touch_down=false; // ditto
        onex_run_evaluators(useruid, 0);
      }
      pressed_ts=0;
    }

    // --------------------

    if(touch_pending){
      touch_pending=false;
      touch_events_seen++;
      g2d_node_touch_event(touch_down, touch_info.x, touch_info.y);
      onex_run_evaluators(useruid, (void*)1);
    }
  }
}

// ------------------- evaluators ----------------

// XXX separated out to remind me to extend the respective APIs to allow sprintf-style
// XXX varargs for path segments, value construction and g2d_node_text()
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

#define BATTERY_ZERO_PERCENT 3400
#define BATTERY_100_PERCENT 4000
#define BATTERY_PERCENT_STEPS 2
bool evaluate_battery_in(object* o, void* d)
{
  int32_t bv = gpio_read(ADC_CHANNEL);
  int32_t mv = bv*2000/(1024/(33/10));
  int16_t pc = ((mv-BATTERY_ZERO_PERCENT)*100/((BATTERY_100_PERCENT-BATTERY_ZERO_PERCENT)*BATTERY_PERCENT_STEPS))*BATTERY_PERCENT_STEPS;
  if(pc<0) pc=0;
  if(pc>100) pc=100;
  snprintf(valuebuf, 64, "%d%%(%ld)", pc, mv);

  object_property_set(battery, "percent", valuebuf);

  int batt=gpio_get(CHARGE_SENSE);
  snprintf(valuebuf, 64, "%s", batt? "powering": "charging");
  object_property_set(battery, "status", valuebuf);

  return true;
}

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

bool evaluate_about_in(object* o, void* d)
{
  snprintf(valuebuf, 64, "%lu %lu", (unsigned long)&__BUILD_TIME, (unsigned long)&__BOOTLOADER_NUMBER);
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


void show_touch_point(uint8_t g2d_node){
  uint8_t touch_g2d_node = g2d_node_create(g2d_node, touch_info.x, touch_info.y, 5,5, 0,0);
  g2d_node_rectangle(touch_g2d_node, 0,0, 5,5, G2D_MAGENTA);
}

static uint8_t fps = 111;

static void draw_by_type(char* p, uint8_t g2d_node);
static void draw_watch(char* p, uint8_t g2d_node);
static void draw_notes(char* p, uint8_t g2d_node);
static void draw_about(char* p, uint8_t g2d_node);
static void draw_list(char* p, uint8_t g2d_node);
static void draw_default(char* p, uint8_t g2d_node);

bool evaluate_user(object* o, void* d) {

  bool is_a_touch_triggered_eval=!!d;

  if(touch_down && !is_a_touch_triggered_eval) return true;

  if(!user_active) return true;

  if(button_action==BUTTON_ACTION_SHORT){
    object_property_set(user, "viewing", watchuid);
    button_action=BUTTON_ACTION_NONE;
  }
  else
  if(button_action==BUTTON_ACTION_LONG){
    object_property_set(user, "viewing", homeuid);
    button_action=BUTTON_ACTION_NONE;
  }

  uint8_t root_g2d_node = g2d_node_create(0, 0,0, ST7789_WIDTH,ST7789_HEIGHT, 0,0);

  g2d_clear_screen(0x00);

  draw_by_type("viewing", root_g2d_node);

  g2d_render();

  uint64_t post_render_time=time_ms();

  static uint8_t renders = 0;
  renders++;

  static uint64_t last_render_time = 0;

  uint64_t time_since_last = post_render_time - last_render_time;
  if(time_since_last > 1000){
     fps = renders * 1000 / time_since_last;
     renders = 0;
     last_render_time = post_render_time;
  }

  return true;
}

void draw_by_type(char* p, uint8_t g2d_node)
{
  snprintf(pathbuf, 64, "%s:is", p);

  if(object_property_contains(user, pathbuf, "watch")) draw_watch(p, g2d_node);   else
  if(object_property_contains(user, pathbuf, "note" )) draw_notes(p, g2d_node);   else
  if(object_property_contains(user, pathbuf, "about")) draw_about(p, g2d_node);   else
  if(object_property_contains(user, pathbuf, "list"))  draw_list(p, g2d_node);    else
                                                       draw_default(p, g2d_node);
}

static int16_t  scroll_bot_lim=0;
static bool     scroll_top=false;
static bool     scroll_bot=false;
static bool     scrolling=false;
static int16_t  scroll_offset=0;

void list_cb(bool down, int16_t dx, int16_t dy, void* uid){

  if(down){
    if(dx+dy){
      scrolling=true;
      bool stretching = (scroll_top && dy>0) ||
                        (scroll_bot && dy<0);
      scroll_offset+= stretching? dy/3: dy;
    }
    return;
  }
  if(scrolling){
    scrolling=false;
    if(scroll_top) scroll_offset=0;
    if(scroll_bot) scroll_offset=scroll_bot_lim;
    return;
  }
  if(uid) object_property_set(user, "viewing", (char*)uid);
}

static char pathbufrec[64];

void draw_list(char* p, uint8_t g2d_node) {

  snprintf(pathbuf, 64, "%s:list", p);

  uint8_t ll=object_property_length(user, pathbuf);

  #define CHILD_HEIGHT 70
  #define BOTTOM_MARGIN 20

  uint16_t scroll_height=10+ll*CHILD_HEIGHT+10;

  scroll_bot_lim = -scroll_height + ST7789_HEIGHT - BOTTOM_MARGIN;
  scroll_top = (scroll_offset > 0);
  scroll_bot = (scroll_offset < scroll_bot_lim);

  uint8_t scroll_g2d_node = g2d_node_create(g2d_node,
                                            0, scroll_offset,
                                            g2d_node_width(g2d_node),
                                            scroll_height,
                                            list_cb, 0);

  uint16_t stretch_height=g2d_node_height(g2d_node)/3;
  if(scroll_top) g2d_node_rectangle(g2d_node, 20,0,                200,stretch_height, G2D_GREY_1D);
  if(scroll_bot) g2d_node_rectangle(g2d_node, 20,2*stretch_height, 200,stretch_height, G2D_GREY_1D);

  uint16_t y=10;

  for(uint8_t i=1; i<=ll; i++){

    snprintf(pathbufrec, 64, "%s:list:%d", p, i);
    // XXX all goes wrong if this recurses (list inside a list)

    char* uid=object_property(user, pathbufrec);

    uint8_t child_g2d_node = g2d_node_create(scroll_g2d_node, 20,y, 200,CHILD_HEIGHT-10, list_cb, uid);

    if(child_g2d_node) draw_by_type(pathbufrec, child_g2d_node);

    y+=CHILD_HEIGHT;
  }
}

#define BATTERY_LOW      G2D_RED
#define BATTERY_MED      G2D_YELLOW
#define BATTERY_HIGH     G2D_GREEN
#define BATTERY_CHARGING G2D_BLUE

void draw_watch(char* path, uint8_t g2d_node)
{
  snprintf(pathbuf, 64, "%s:battery:percent", path);
  char* pc=object_property(   user, pathbuf);
  snprintf(pathbuf, 64, "%s:battery:status", path);
  bool  ch=object_property_is(user, pathbuf, "charging");
  snprintf(pathbuf, 64, "%s:watchface:clock:ts", path);
  char* ts=object_property(   user, pathbuf);
  snprintf(pathbuf, 64, "%s:watchface:clock:tz:2", path);
  char* tz=object_property(   user, pathbuf);
  snprintf(pathbuf, 64, "%s:watchface:ampm-24hr", path);
  bool h24=object_property_is(user, pathbuf, "24hr");

  if(!ts) return;

  char* e;

  uint64_t tsnum=strtoull(ts,&e,10);
  if(*e) return;

  uint32_t tznum=strtoul(tz?tz:"0",&e,10);
  if(*e) tznum=0;

  time_t est=(time_t)(tsnum+tznum);
  struct tm tms={0};
  localtime_r(&est, &tms);

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    g2d_node_rectangle(g2d_node, 0,0, g2d_node_width(g2d_node),g2d_node_height(g2d_node), G2D_YELLOW/6);
    strftime(g2dbuf, 64, h24? "%H:%M": "%l:%M", &tms);
    g2d_node_text(g2d_node, 10,20, g2dbuf, G2D_WHITE, G2D_YELLOW/6, 3);
    return;
  }

  strftime(g2dbuf, 64, h24? "%H:%M": "%l:%M", &tms);
  g2d_node_text(g2d_node, 10, 90, g2dbuf, G2D_BLUE, G2D_BLACK, 7);

  if(!h24){
    strftime(g2dbuf, 64, "%p", &tms);
    g2d_node_text(g2d_node, 100, 150, g2dbuf, G2D_BLUE, G2D_BLACK, 3);
  }

  strftime(g2dbuf, 64, "%a %d %h", &tms);
  g2d_node_text(g2d_node, 30, 210, g2dbuf, G2D_BLUE, G2D_BLACK, 3);

  int8_t pcnum=pc? (int8_t)strtol(pc,&e,10): 0;
  if(pcnum<0) pcnum=0;
  if(pcnum>100) pcnum=100;

  uint16_t batt_col;
  if(ch)       batt_col=BATTERY_CHARGING; else
  if(pcnum>67) batt_col=BATTERY_HIGH;     else
  if(pcnum>33) batt_col=BATTERY_MED;
  else         batt_col=BATTERY_LOW;

  snprintf(g2dbuf, 64, "%d", pcnum);
  g2d_node_text(g2d_node, 10, 30, g2dbuf, batt_col, G2D_BLACK, 3);
}

// ---------------------- keyboard ------------------------

static char    typed[64];
static uint8_t cursor=0;

void del_char()
{
  if(cursor==0) return;
  typed[--cursor]=0;
}

void add_char(unsigned char c)
{
  if(cursor==62) return;
  typed[cursor++]=c;
  typed[cursor]=0;
}

static uint8_t kbpg=1;

static unsigned char key_pages[7][20]={

  { "     " },
  { "ERTIO"
    "ASDGH"
    "CBNM>"
    "^#  ~" },
  { "QWYUP"
    "FJKL "
    " ZXV<"
    "^#  ~" },
  { "+ ()-"
    "/'#@*"
    "%!;:>"
    "^#,.~" },
  { "^&[]_"
    " \"{}~"
    " ?<><"
    "^#,.~" },
  { "+123-"
    "/456*"
    "%789>"
    "^#0.=" },
  { "^123e"
    " 456 "
    " 789<"
    "^#0.=" }
};


#define SELECT_TYPE 15
#define SELECT_PAGE 14
#define DELETE_LAST 16

void key_hit(bool down, int16_t dx, int16_t dy, void* kiv){

  if(down) return;

  uint32_t ki=(uint32_t)kiv;

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
  else {

    add_char(key_pages[kbpg][ki]);

    if(kbpg==2 || kbpg==6) kbpg--;
    else
    if(kbpg==3 || kbpg==4) kbpg=1;
  }
}

#define KEY_SIZE  41
#define KEY_H_SPACE 7
#define KEY_V_SPACE 1

static void build_keyboard(uint8_t kbd_g2d_node){

  uint16_t kx=0;
  uint16_t ky=30;

  unsigned char pressed=0;

  for(uint8_t j=0; j<4; j++){
    for(uint8_t i=0; i<5; i++){

      uint32_t ki = i + j*5;

      unsigned char key = key_pages[kbpg][ki];

      uint8_t key_g2d_node = g2d_node_create(kbd_g2d_node,
                                             kx, ky,
                                             KEY_SIZE+KEY_H_SPACE, KEY_SIZE+KEY_V_SPACE,
                                             key_hit, (void*)ki);

      uint16_t key_bg=(pressed==key)? G2D_GREEN: G2D_GREY_1A;

      g2d_node_rectangle(key_g2d_node, KEY_H_SPACE/2,KEY_V_SPACE/2, KEY_SIZE,KEY_SIZE, key_bg);

      snprintf(g2dbuf, 64, "%c", key);
      g2d_node_text(key_g2d_node, 13,7, g2dbuf, G2D_BLACK, key_bg, 4);

      kx+=KEY_SIZE+KEY_H_SPACE;
    }
    kx=0;
    ky+=KEY_SIZE+KEY_V_SPACE;
  }
}

// --------------------------------------------------------

#define KBDSTART_X 0
#define KBDSTART_Y 75

static int16_t kbd_x=KBDSTART_X;
static int16_t kbd_y=KBDSTART_Y;

void kbd_drag(bool down, int16_t dx, int16_t dy, void* arg){
  if(!down || dx+dy==0) return;
  kbd_x+=dx;
  kbd_y+=dy;
}

void draw_notes(char* path, uint8_t g2d_node) {

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    g2d_node_rectangle(g2d_node, 0,0, g2d_node_width(g2d_node),g2d_node_height(g2d_node), G2D_GREEN/6);
    snprintf(g2dbuf, 64, "%s|", typed);
    g2d_node_text(g2d_node, 10,20, g2dbuf, G2D_WHITE, G2D_GREEN/6, 2);
    return;
  }

  uint8_t typed_g2d_node = g2d_node_create(g2d_node, 5,5, 200,80, 0,0);

  snprintf(g2dbuf, 64, "fps: %02d (%d,%d)", fps, touch_info.x, touch_info.y);
  g2d_node_text(typed_g2d_node, 10,5, g2dbuf, G2D_BLUE, G2D_BLACK, 2);

  snprintf(g2dbuf, 64, "%s|", typed);
  g2d_node_text(typed_g2d_node, 5,25, g2dbuf, G2D_BLUE, G2D_BLACK, 2);

  uint8_t kbd_g2d_node = g2d_node_create(g2d_node,
                                         kbd_x, kbd_y,
                                         g2d_node_width(g2d_node), 215,
                                         kbd_drag, 0);
  g2d_node_rectangle(kbd_g2d_node, 0,0,
                     g2d_node_width(kbd_g2d_node),g2d_node_height(kbd_g2d_node),
                     G2D_GREY_F);

  build_keyboard(kbd_g2d_node);

  show_touch_point(g2d_node);
}

void draw_about(char* path, uint8_t g2d_node) {

  snprintf(pathbuf, 64, "%s:cpu", path);
  char* cpu=object_property(user, pathbuf);

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    g2d_node_rectangle(g2d_node, 0,0, g2d_node_width(g2d_node),g2d_node_height(g2d_node), G2D_CYAN/6);
//  uint32_t touch_events_percent = (100*touch_events_seen)/(1+touch_events);
//  snprintf(g2dbuf, 64, "%ld%% %ldms", touch_events_percent, loop_time);
    snprintf(g2dbuf, 64, "%dfps %ldms", fps, loop_time);
    g2d_node_text(g2d_node, 10,20, g2dbuf, G2D_WHITE, G2D_CYAN/6, 3);
    return;
  }

  snprintf(pathbuf, 64, "%s:build-info", path);
  char* bnf=object_property_values(user, pathbuf);

  snprintf(g2dbuf, 64, "fps: %d (%d,%d)", fps, touch_info.x, touch_info.y);
  g2d_node_text(g2d_node, 20, 20, g2dbuf, G2D_BLUE, G2D_BLACK, 2);

  snprintf(g2dbuf, 64, "cpu: %s", cpu);
  g2d_node_text(g2d_node, 10, 70, g2dbuf, G2D_BLUE, G2D_BLACK, 3);

  snprintf(g2dbuf, 64, "build: %s", bnf);
  g2d_node_text(g2d_node, 10, 190, g2dbuf, G2D_BLUE, G2D_BLACK, 1);
}

void draw_default(char* path, uint8_t g2d_node)
{
  snprintf(pathbuf, 64, "%s:is", path);
  char* is=object_property_values(user, pathbuf);
  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    g2d_node_rectangle(g2d_node, 0,0, g2d_node_width(g2d_node),g2d_node_height(g2d_node), G2D_MAGENTA);
    g2d_node_text(g2d_node, 10,20, is, G2D_BLACK, G2D_MAGENTA, 3);
    return;
  }
  g2d_node_text(g2d_node, 10, 190, "no show", G2D_BLUE, G2D_BLACK, 1);
}

#if defined(LOG_TO_GFX)
void draw_log()
{
  // render (const char*)event_log_buffer);
}
#endif

