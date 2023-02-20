
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <boards.h>
#include <items.h>
#include <onex-kernel/boot.h>
#include <onex-kernel/log.h>
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

#include "mathlib.h"
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
static object* note1;
static object* note2;
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
static char* note1uid;
static char* note2uid;
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
    log_write("eval user from touched\n");
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
#endif

extern char __BUILD_TIMESTAMP;
extern char __BUILD_TIME;
extern char __BOOTLOADER_NUMBER;

static uint32_t loop_time=0;

int main() {

  boot_init();
  log_init();
  time_init_set((unsigned long)&__BUILD_TIMESTAMP);
  gpio_init();

  set_up_gpio();

  touch_init(touched);
#if defined(DO_LATER)
  motion_init(moved);
#endif

  g2d_init();

  onex_init("");

  //                               setters and editors                         inputs              logic                                outputs
  onex_set_evaluators("default",   evaluate_object_setter,                                         evaluate_default, 0);
  onex_set_evaluators("editable",  evaluate_object_setter, evaluate_edit_rule, 0);
  onex_set_evaluators("clock",     evaluate_object_setter,                                         evaluate_clock_sync, evaluate_clock, 0);
  onex_set_evaluators("device",                                                                    evaluate_device_logic, 0);
  onex_set_evaluators("user",                                                                      evaluate_user, 0);
  onex_set_evaluators("notes",     evaluate_object_setter, evaluate_edit_rule, 0);
  onex_set_evaluators("battery",                                               evaluate_battery_in, 0);
  onex_set_evaluators("touch",                                                 evaluate_touch_in, 0);
#if defined(DO_LATER)
  onex_set_evaluators("motion",                                                evaluate_motion_in, 0);
#endif
  onex_set_evaluators("button",                                                evaluate_button_in, 0);
  onex_set_evaluators("about",                                                 evaluate_about_in, 0);
  onex_set_evaluators("backlight", evaluate_object_setter, evaluate_edit_rule,                     evaluate_light_logic,                evaluate_backlight_out, 0);

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
  note1    =object_new(0, "notes",     "text editable", 4);
  note2    =object_new(0, "notes",     "text editable", 4);
  notes    =object_new(0, "notes",     "text list editable", 4);
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
  note1uid    =object_property(note1, "UID");
  note2uid    =object_property(note2, "UID");
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

  static char note_text[] = "the fat cat sat on me";

  static char note_text_big[] =
                            "xxxxxxxxxxxxxxxxxxx "
                            "xxxxxxxxxxxxxxxxxx "
                            "xxxxxxxxxxxxxxxx "
                            "xxxxxxxxxxxxxxx "
                            "xxxxxxxxxxxxxx "
                            "xxxxxxxxxxxxx "
                            "xxxxxxxxxxxx "
                            "xxxxxxxxxxx "
                            "xxxxxxxxxx "
                            "xxxxxxxxx "
                            "xxxxxxxx "
                            "xxxxxxx "
                            "xxxxxx "
                            "xxxxx "
                            "xxxx "
                            "xxx "
                            "xx "
                            "x "
                            "Welcome to OnexOS! "
                            "and the Object Network "
                            "A Smartwatch OS "
                            "Without Apps "
                            "app-killer, inversion "
                            "only see data in HX "
                            "no apps, like the Web "
                            "all our data "
                            "just stuff - objects "
                            "you can link to and list "
                            "little objects "
                            "of all kinds of data "
                            "linked together "
                            "semantic "
                            "on our own devices "
                            "hosted by you (including you) "
                            "sewn together "
                            "into a global, shared fabric "
                            "which we can all Link up "
                            "into a shared global data fabric "
                            "like the Web "
                            "mesh "
                            "see objects inside other watches "
                            "add their objects to your lists "
                            "internet after mesh "
                            "we create a global data fabric "
                            "from all our objects linked up "
                            "a two-way dynamic data Web "
                            "a global Meshaverse "
                            "chat Freedom Meshaverse "
                            "spanning the planet "
                            "animated by us "
                            "internally-animated "
                            "programmed like a spreadsheet "
                            "objects are live - you see them change "
                            "you have live presence as an object yourself "
                            "SS-like PL over objects as objects themselves "
                            "can share rule object set objects "
                            "like 'downloading an app' "
                            "internally-animated "
                            "with behaviour rules you can write yourself "
                            "and animate ourselves with spreadsheet-like rules "
                            "----- ----- ----- ----- -----";
  char* strtok_state = 0;
  char* word = strtok_r(note_text, " ", &strtok_state);
  while(word){
    object_property_add(note1, "text", word);
    word = strtok_r(0, " ", &strtok_state);
  }
  strtok_state = 0;
  word = strtok_r(note_text_big, " ", &strtok_state);
  while(word){
    object_property_add(note2, "text", word);
    word = strtok_r(0, " ", &strtok_state);
  }
  object_property_add(notes, "list", note1uid);
  object_property_add(notes, "list", note2uid);

  object_property_add(home, "list", notesuid);
  object_property_add(home, "list", aboutuid);
  object_property_add(home, "list", note1uid);
  object_property_add(home, "list", note2uid);
  object_property_add(home, "list", watchuid);
  object_property_add(home, "list", clockuid);
  object_property_add(home, "list", homeuid);
  object_property_add(home, "list", useruid);
  object_property_add(home, "list", watchfaceuid);
  object_property_add(home, "list", backlightuid);
  object_property_add(home, "list", batteryuid);
  object_property_add(home, "list", touchuid);
  object_property_add(home, "list", buttonuid);
#if defined(DO_LATER)
  object_property_add(home, "list", motionuid);
#endif

  object_property_set(watch, "battery",   batteryuid);
  object_property_set(watch, "watchface", watchfaceuid);

  object_property_set(user, "viewing", watchuid);

  object_property_add(onex_device_object, "user", useruid);
  object_property_add(onex_device_object, "io",   batteryuid);
  object_property_add(onex_device_object, "io",   touchuid);
#if defined(DO_LATER)
  object_property_add(onex_device_object, "io",   motionuid);
#endif
  object_property_add(onex_device_object, "io",   buttonuid);
  object_property_add(onex_device_object, "io",   backlightuid);
  object_property_add(onex_device_object, "io",   clockuid);

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
  log_write("evaluate_default data=%p\n", d); object_log(o);
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

  uint8_t batt=gpio_get(CHARGE_SENSE);
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

static void eval_update_list(char* uid, char* key, uint16_t i, char* val) {
  properties* update = properties_new(1);
  list* li=0;
  if(i){
    li=list_new(i+1);
    if(!val || !*val){
      for(uint16_t x=1; x<i; x++) list_add(li, value_new("something"));
      list_add(li, value_new("(=>)"));
    }
  }
  else{
    if(val && *val){
      li=list_new(3);
      list_add(li, value_new((char*)"=>"));
      list_add(li, value_new((char*)"@."));
      list_add(li, value_new(val));
    }
  }
  properties_set(update, key, li);
  onex_run_evaluators(uid, update);
}

static object* create_new_object_like_others(char* path) {
  object* r=0;
  int16_t ll = object_property_length(user, path);

  int8_t maxsz=0;
  for(int i=1; i<=ll; i++){
    snprintf(pathbuf, 64, "%s:%d:", path, i);
    int8_t sz=object_property_size(user, pathbuf);
    if(sz>maxsz) maxsz=sz;
  }
  if(maxsz<4) maxsz=4;

  return r;
}

static void show_touch_point(uint8_t g2d_node){
  uint8_t touch_g2d_node = g2d_node_create(g2d_node, touch_info.x, touch_info.y, 5,5, 0,0);
  g2d_node_rectangle(touch_g2d_node, 0,0, 5,5, G2D_MAGENTA);
}

static char* list_selected_uid=0;

static uint16_t del_this_word=0;
static char*    add_this_word=0;

static int16_t  scroll_bot_lim=0;
static bool     scroll_top=false;
static bool     scroll_bot=false;
static bool     scrolling=false;
static int16_t  scroll_offset=0;

static uint16_t word_index=1;
static bool     in_word=false;
static char     edit_word[64];
static uint8_t  cursor=0;

#define KBDSTART_X 0
#define KBDSTART_Y 200
static uint8_t kbd_page=1;
static int16_t kbd_x=KBDSTART_X;
static int16_t kbd_y=KBDSTART_Y;

static int16_t text_scroll_offset=0;

static void reset_viewing_state_variables(){

  scroll_bot_lim=0;
  scroll_top=false;
  scroll_bot=false;
  scrolling=false;
  scroll_offset=0;

  word_index=1;
  in_word=false;
  edit_word[0]=0;
  cursor=0;

  kbd_page=1;
  kbd_x=KBDSTART_X;
  kbd_y=KBDSTART_Y;

  text_scroll_offset=0;
}

static uint8_t fps = 111;

static void draw_by_type(char* p, uint8_t g2d_node);
static void draw_list(char* p, uint8_t g2d_node);
static void draw_watch(char* p, uint8_t g2d_node);
static void draw_notes(char* p, uint8_t g2d_node);
static void draw_about(char* p, uint8_t g2d_node);
static void draw_default(char* p, uint8_t g2d_node);

static bool evaluate_user(object* o, void* d) {

  bool is_a_touch_triggered_eval=!!d;

  if(touch_down && !is_a_touch_triggered_eval) return true;

  if(!user_active) return true;

  if(button_action==BUTTON_ACTION_SHORT){
    uint16_t histlen=object_property_length(user, "history");
    if(histlen){
      snprintf(pathbuf, 64, "history:%d", histlen);
      char* viewing = object_property(user, pathbuf);
      object_property_set(user, pathbuf, 0);
      object_property_set(user, "viewing", viewing);
      reset_viewing_state_variables();
    }
    button_action=BUTTON_ACTION_NONE;
  }
  else
  if(button_action==BUTTON_ACTION_LONG){
    char* viewing_uid=object_property(user, "viewing");
    object_property_add(user, "history", viewing_uid);
    object_property_set(user, "viewing", homeuid);
    reset_viewing_state_variables();
    button_action=BUTTON_ACTION_NONE;
  }

  if(list_selected_uid){
    char* viewing_uid=object_property(user, "viewing");
    if(!strcmp(list_selected_uid, "new-at-top")){
      object* o = create_new_object_like_others("viewing:list");
      if(o) eval_update_list(viewing_uid, "list", 0, object_property(o, "UID"));
    }
    else{
      object_property_add(user, "history", viewing_uid);
      object_property_set(user, "viewing", list_selected_uid);
      reset_viewing_state_variables();
    }
    list_selected_uid=0;
  }

  if(del_this_word || add_this_word){
    char* viewing_uid=object_property(user, "viewing");
    eval_update_list(viewing_uid, "text", del_this_word, add_this_word);
    del_this_word=0; add_this_word=0;
  }

  uint8_t root_g2d_node = g2d_node_create(0, 0,0, ST7789_WIDTH,ST7789_HEIGHT, 0,0);

  g2d_clear_screen(0x00);

  draw_by_type("viewing", root_g2d_node);

#if defined(LOG_TO_GFX)
  uint64_t pre_render_time=time_ms();

  #define LOG_LINES_MAX 5
  #define LOG_LEN 40
  static char*    log_lines[LOG_LINES_MAX];
  static uint8_t  log_lines_index=0;
  static uint64_t log_last=0;
  if(event_log_buffer){
    if(log_lines[log_lines_index]) free(log_lines[log_lines_index]);
    log_lines[log_lines_index]=strndup((char*)event_log_buffer, LOG_LEN);
    log_lines_index=(log_lines_index+1)%LOG_LINES_MAX;
    uint8_t linelen=strlen((char*)event_log_buffer);
    if(linelen>LOG_LEN){
      char* lastbit=(char*)event_log_buffer+linelen-LOG_LEN;
      log_lines[log_lines_index]=strndup(lastbit, LOG_LEN);
      log_lines_index=(log_lines_index+1)%LOG_LINES_MAX;
    }
    event_log_buffer=0;
    log_last=pre_render_time;
  }
  else
  if(log_last && pre_render_time > log_last+2000){
    for(uint8_t i=0; i<LOG_LINES_MAX-1; i++){
      log_lines[i]=log_lines[i+1];
    }
    log_lines[LOG_LINES_MAX-1]=0;
    log_lines_index=max(log_lines_index-1, 0);
    log_last=pre_render_time-1;
  }
  for(uint8_t i=0; i<LOG_LINES_MAX; i++){
    if(log_lines[i]){
      g2d_node_text(root_g2d_node, 20,8+i*8, log_lines[i], G2D_RED, G2D_BLACK, 1);
    }
  }
#endif

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

  if(object_property_contains(user, pathbuf, "list"))  draw_list(p, g2d_node);    else
  if(object_property_contains(user, pathbuf, "watch")) draw_watch(p, g2d_node);   else
  if(object_property_contains(user, pathbuf, "text" )) draw_notes(p, g2d_node);   else
  if(object_property_contains(user, pathbuf, "about")) draw_about(p, g2d_node);   else
                                                       draw_default(p, g2d_node);
}

static void list_cb(bool down, int16_t dx, int16_t dy, void* uid){

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
  list_selected_uid=uid;
}

static void draw_list(char* p, uint8_t g2d_node) {

  snprintf(pathbuf, 64, "%s:list", p);

  uint8_t ll=object_property_length(user, pathbuf);

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    g2d_node_rectangle(g2d_node, 0,0, g2d_node_width(g2d_node),g2d_node_height(g2d_node), G2D_GREY_1D/13);
    g2d_node_text(g2d_node, 10,20, "list", G2D_WHITE, G2D_GREY_1D/13, 3);
    return;
  }
  #define CHILD_HEIGHT 70
  #define BOTTOM_MARGIN 20

  uint16_t scroll_height=max(10+ll*CHILD_HEIGHT+10, ST7789_HEIGHT*4/3);

  uint8_t scroll_g2d_node = g2d_node_create(g2d_node,
                                            0, scroll_offset,
                                            g2d_node_width(g2d_node),
                                            scroll_height,
                                            list_cb, 0);
  if(!scroll_g2d_node) return;

  scroll_bot_lim = -scroll_height + ST7789_HEIGHT - BOTTOM_MARGIN;
  scroll_top = (scroll_offset > 0);
  scroll_bot = (scroll_offset < scroll_bot_lim);

  uint16_t stretch_height=g2d_node_height(g2d_node)/3;
  if(scroll_top) g2d_node_rectangle(g2d_node,
                                    20,0,
                                    200,stretch_height, G2D_GREY_7);
  if(scroll_bot) g2d_node_rectangle(g2d_node,
                                    20,2*stretch_height,
                                    200,stretch_height, G2D_GREY_7);

  uint8_t new_top_g2d_node = g2d_node_create(scroll_g2d_node,
                                             20,10,
                                             200,CHILD_HEIGHT-30,
                                             list_cb, "new-at-top");
  g2d_node_rectangle(new_top_g2d_node, 0,0,
                     g2d_node_width(new_top_g2d_node),g2d_node_height(new_top_g2d_node),
                     G2D_GREY_F);
  g2d_node_text(new_top_g2d_node, 10,10, "add new", G2D_WHITE, G2D_GREY_F, 2);

  uint16_t y=g2d_node_height(new_top_g2d_node)+20;

  for(uint8_t i=1; i<=ll; i++){

    static char pathbufrec[64];
    snprintf(pathbufrec, 64, "%s:list:%d", p, i);
    char* uid=object_property(user, pathbufrec);

    uint8_t child_g2d_node = g2d_node_create(scroll_g2d_node,
                                             20,y,
                                             200,CHILD_HEIGHT-10,
                                             list_cb, uid);

    if(child_g2d_node) draw_by_type(pathbufrec, child_g2d_node);

    y+=CHILD_HEIGHT;
  }
}

#define BATTERY_LOW      G2D_RED
#define BATTERY_MED      G2D_YELLOW
#define BATTERY_HIGH     G2D_GREEN
#define BATTERY_CHARGING G2D_BLUE

static void draw_watch(char* path, uint8_t g2d_node) {

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
  g2d_node_text(g2d_node, 10, 90, g2dbuf, G2D_WHITE, G2D_BLACK, 7);

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

static void del_word(){
  if(word_index==1) return;
  del_this_word=word_index-1;
  add_this_word=0;
  word_index--;
}

static void add_word(){
  del_this_word=0;
  add_this_word=edit_word;
  word_index++;
}

static void del_char() {
  if(in_word){
    if(cursor==0) return;
    edit_word[--cursor]=0;
    if(cursor==0) in_word=false;
    return;
  }
  del_word();
}

static void add_char(unsigned char c) {
  if(c!=' '){
    if(cursor==62) return;
    in_word=true;
    edit_word[cursor++]=c;
    edit_word[cursor]=0;
    return;
  }
  if(in_word) add_word();
  in_word=false;
  cursor=0;
}

static unsigned char kbd_pages[7][20]={

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

static void key_hit(bool down, int16_t dx, int16_t dy, void* kiv){

  if(down) return;

  uint32_t ki=(uint32_t)kiv;

  if(ki==SELECT_TYPE){
    if(kbd_page==1 || kbd_page==2) kbd_page=3;
    else
    if(kbd_page==3 || kbd_page==4) kbd_page=5;
    else
    if(kbd_page==5 || kbd_page==6) kbd_page=1;
  }
  else
  if(ki==SELECT_PAGE){
    if(kbd_page==1) kbd_page=2;
    else
    if(kbd_page==2) kbd_page=1;
    else
    if(kbd_page==3) kbd_page=4;
    else
    if(kbd_page==4) kbd_page=3;
    else
    if(kbd_page==5) kbd_page=6;
    else
    if(kbd_page==6) kbd_page=5;
  }
  else
  if(ki==DELETE_LAST){

    del_char();
  }
  else {

    add_char(kbd_pages[kbd_page][ki]);

    if(kbd_page==2 || kbd_page==6) kbd_page--;
    else
    if(kbd_page==3 || kbd_page==4) kbd_page=1;
  }
}

#define KEY_SIZE  41
#define KEY_H_SPACE 7
#define KEY_V_SPACE 1

static void kbd_drag(bool down, int16_t dx, int16_t dy, void* arg){
  if(!down || dx+dy==0) return;
  if(dx*dx>dy*dy) kbd_x+=dx;
  else            kbd_y+=dy;
}

static void show_keyboard(uint8_t g2d_node){

  uint8_t kbd_g2d_node = g2d_node_create(g2d_node,
                                         kbd_x, kbd_y,
                                         g2d_node_width(g2d_node), 215,
                                         kbd_drag, 0);
  if(!kbd_g2d_node) return;

  g2d_node_rectangle(kbd_g2d_node, 0,0,
                     g2d_node_width(kbd_g2d_node),g2d_node_height(kbd_g2d_node),
                     G2D_GREY_F);

  uint16_t kx=0;
  uint16_t ky=30;

  unsigned char pressed=0;

  for(uint8_t j=0; j<4; j++){
    for(uint8_t i=0; i<5; i++){

      uint32_t ki = i + j*5;

      unsigned char key = kbd_pages[kbd_page][ki];

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

static void word_cb(bool down, int16_t dx, int16_t dy, void* wi){
  static bool scrolled=false;
  if(!down){
    if(!scrolled && wi) word_index=(uint16_t)(uint32_t)wi;
    scrolled=false;
  }
  else
  if(dx+dy!=0){
    text_scroll_offset+=dy;
    scrolled=true;
  }
}

static void draw_notes(char* path, uint8_t g2d_node) {

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    snprintf(pathbuf, 64, "%s:text:1", path);
    char* line1=object_property(user, pathbuf);
    snprintf(pathbuf, 64, "%s:text:2", path);
    char* line2=object_property(user, pathbuf);
    g2d_node_rectangle(g2d_node, 0,0, g2d_node_width(g2d_node),g2d_node_height(g2d_node), G2D_GREEN/6);
    g2d_node_text(g2d_node, 10,20, line1, G2D_WHITE, G2D_GREEN/6, 2);
    g2d_node_text(g2d_node, 10,40, line2, G2D_WHITE, G2D_GREEN/6, 2);
    return;
  }

  #define SIDE_MARGIN  20
  #define TOP_MARGIN   30
  #define LINE_HEIGHT  20
  #define WORD_SPACING  7
  #define CURSOR_WIDTH  3

  snprintf(g2dbuf, 64, "fps: %02d (%d,%d)", fps, touch_info.x, touch_info.y);
  g2d_node_text(g2d_node, 10,5, g2dbuf, G2D_BLUE, G2D_BLACK, 2);

  int16_t wd=g2d_node_width(g2d_node)-2*SIDE_MARGIN;
  int16_t ht=g2d_node_height(g2d_node)-2*TOP_MARGIN;
  if(wd<0 || ht<0) return;

  uint8_t text_container_g2d_node = g2d_node_create(g2d_node,
                                                    SIDE_MARGIN,TOP_MARGIN, wd,ht,
                                                    word_cb, 0);
  if(!text_container_g2d_node) return;

  snprintf(pathbuf, 64, "%s:text", path);
  uint16_t words=object_property_length(user, pathbuf);

  uint16_t lines=(words/2)+1;
  uint16_t scroll_height=lines*LINE_HEIGHT;

  if(in_word){
    word_index=words+1;
  }

  uint8_t text_scroll_g2d_node = g2d_node_create(text_container_g2d_node,
                                                 0, text_scroll_offset,
                                                 wd, scroll_height,
                                                 word_cb, 0);
  if(text_scroll_g2d_node){
    uint16_t k=0;
    uint16_t j=0;
    for(uint16_t w=1; w<=words+1; w++){

      int16_t available_width = wd-k;

      char*    word = 0;
      uint16_t word_width=available_width;
      if(w<=words){
        word = object_property_get_n(user, pathbuf, w);
        word_width=WORD_SPACING+g2d_text_width(word, 2);
      }
      if(k && word_width > available_width){
        // not fab way to "extend" the prev word out to full width for cursor
        g2d_node_create(text_scroll_g2d_node,
                        k, j*LINE_HEIGHT,
                        available_width, LINE_HEIGHT,
                        word_cb,(void*)(uint32_t)(w-1));
        k=0;
        j++;
      }
      if(w==word_index){
        g2d_node_rectangle(text_scroll_g2d_node,
                           k, j*LINE_HEIGHT,
                           CURSOR_WIDTH, LINE_HEIGHT,
                           G2D_MAGENTA);
      }

      uint8_t word_g2d_node = g2d_node_create(text_scroll_g2d_node,
                                              k, j*LINE_HEIGHT,
                                              word_width, LINE_HEIGHT,
                                              word_cb,(void*)(uint32_t)w);

      char* word_to_show = word? word: (in_word? edit_word: "");
      g2d_node_text(word_g2d_node, 6,2, word_to_show, G2D_WHITE, G2D_BLACK, 2);
      k+=word_width;
    }
  }

  show_keyboard(g2d_node);

  show_touch_point(g2d_node);
}

static void draw_about(char* path, uint8_t g2d_node) {

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
  g2d_node_text(g2d_node, 20, 40, g2dbuf, G2D_BLUE, G2D_BLACK, 2);

  snprintf(g2dbuf, 64, "cpu: %s", cpu);
  g2d_node_text(g2d_node, 10, 110, g2dbuf, G2D_BLUE, G2D_BLACK, 3);

  snprintf(g2dbuf, 64, "build: %s", bnf);
  g2d_node_text(g2d_node, 10, 190, g2dbuf, G2D_BLUE, G2D_BLACK, 1);
}

void draw_default(char* path, uint8_t g2d_node) {

  snprintf(pathbuf, 64, "%s:is", path);
  char* is=object_property_values(user, pathbuf);

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    g2d_node_rectangle(g2d_node,
                       0,0,
                       g2d_node_width(g2d_node),g2d_node_height(g2d_node),
                       G2D_MAGENTA);
    g2d_node_text(g2d_node, 10,20, is, G2D_BLACK, G2D_MAGENTA, 3);
    return;
  }
  snprintf(pathbuf, 64, "%s", path);
  char* uid=object_property(user, pathbuf);
  object* o=onex_get_from_cache(uid);
  static char bigvaluebuf[256];
  object_to_text(o, bigvaluebuf, 256, OBJECT_TO_TEXT_LOG);

  uint16_t p=0;
  uint16_t l=strlen(bigvaluebuf);
  uint16_t y=30;
  while(y<280 && p < l){
    g2d_node_text(g2d_node, 10, y, bigvaluebuf+p, G2D_BLUE, G2D_BLACK, 2);
    y+=20; p+=19;
  }
}

