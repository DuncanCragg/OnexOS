
#include <stdint.h>

#include <boards.h>

#include <onex-kernel/boot.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/spi.h>
#include <onex-kernel/i2c.h>
#include <onex-kernel/touch.h>

#include <onn.h>
#include <onr.h>

#include "g2d.h"

char* useruid;

static char* batteryuid;
static char* touchuid;
static char* buttonuid;
static char* clockuid;
static char* aboutuid;

char* homeuid;
char* inventoryuid;

object* user;
object* responses;

#define LONG_PRESS_MS 250
bool                   button_pending =false;
volatile bool          button_pressed=false;

static volatile bool   touch_pending=false;

volatile touch_info_t  touch_info={ 120, 140 };
volatile bool          touch_down=false;

#if defined(HAS_MOTION)
volatile motion_info_t motion_info;
#endif

static void every_second(){
  onex_run_evaluators(clockuid, 0);
  onex_run_evaluators(aboutuid, 0);

#if defined(EPOCH_ADJUST)
  static uint16_t period=EPOCH_ADJUST < 0? -EPOCH_ADJUST: EPOCH_ADJUST;
  static  int16_t amount=EPOCH_ADJUST < 0? -1: 1;
  static uint32_t seconds_adjust=0;
  if(period && !(++seconds_adjust % period)) time_es_set(time_es()+amount);
#endif
}

static void every_10s(){
  onex_run_evaluators(batteryuid, 0);
}

volatile uint32_t touch_events=0;
volatile uint32_t touch_events_seen=0;
volatile uint32_t touch_events_spurious=0;

static void touched(touch_info_t ti) {

  // ---------------------------------------
  // XXX this cb is spuriously called by button presses;
  // luckily x+y are set to zero
  if(!(ti.x+ti.y)){ touch_events_spurious++; return; }
  // ---------------------------------------

  // ---------------------------------------
  // XXX maybe need to drive touch chip differently
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

  touch_events++;

  touch_info=ti;

  bool is_down_event = touch_info.action==TOUCH_ACTION_CONTACT;

  if(!touch_down && is_down_event){ if(touch_pending) return; touch_down=true; }
  else
  if(touch_down && !is_down_event){ touch_down=false; touch_pending=true; }

  if(is_down_event) touch_pending=true;

  onex_run_evaluators(touchuid, 0);
}


#if defined(HAS_MOTION)
static void moved(motion_info_t mi)
{
  motion_info=mi;
  onex_run_evaluators(motionuid, 0);
}
#endif

static void button_changed(uint8_t pin, uint8_t type){
  button_pressed = (gpio_get(BUTTON_1)==BUTTONS_ACTIVE_STATE);
  button_pending=true;
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

extern bool user_active;

#if defined(LOG_TO_GFX)
extern volatile char* event_log_buffer;
#endif

extern char __BUILD_TIMESTAMP;

uint32_t loop_time=0;

extern bool evaluate_default(object* o, void* d);
extern bool evaluate_user(object* o, void* d);
extern bool evaluate_battery_in(object* o, void* d);
extern bool evaluate_touch_in(object* o, void* d);
#if defined(HAS_MOTION)
extern bool evaluate_motion_in(object* o, void* d);
#endif
extern bool evaluate_button_in(object* o, void* d);
extern bool evaluate_about_in(object* o, void* d);
extern bool evaluate_backlight_out(object* o, void* d);

static char note_text[] = "the fat cat sat on me";

static char note_text_big[] =
  "xxxxxxxxxxxxxxxxxxx " "xxxxxxxxxxxxxxxxxx " "xxxxxxxxxxxxxxxx " "xxxxxxxxxxxxxxx "
  "xxxxxxxxxxxxxx " "xxxxxxxxxxxxx " "xxxxxxxxxxxx " "xxxxxxxxxxx " "xxxxxxxxxx "
  "xxxxxxxxx " "xxxxxxxx " "xxxxxxx " "xxxxxx " "xxxxx " "xxxx " "xxx " "xx " "x "
  "Welcome to OnexOS! " "and the Object Network " "A Smartwatch OS " "Without Apps "
  "app-killer, inversion " "only see data in HX " "no apps, like the Web " "all our data "
  "just stuff - objects " "you can link to and list " "little objects "
  "of all kinds of data " "linked together " "semantic " "on our own devices "
  "hosted by you (including you) " "sewn together " "into a global, shared fabric "
  "which we can all Link up " "into a shared global data fabric " "like the Web " "mesh "
  "see objects inside other watches " "add their objects to your lists "
  "internet after mesh " "we create a global data fabric "
  "from all our objects linked up " "a two-way dynamic data Web " "a global Meshaverse "
  "chat Freedom Meshaverse " "spanning the planet " "animated by us "
  "internally-animated " "programmed like a spreadsheet "
  "objects are live - you see them change " "you have live presence as an object yourself "
  "SS-like PL over objects as objects themselves " "can share rule object set objects "
  "like 'downloading an app' " "internally-animated "
  "with behaviour rules you can write yourself "
  "and animate ourselves with spreadsheet-like rules "
  "----- ----- ----- ----- -----";

static void init_onex(){

  onex_init("");

  //                               editor    inputs    logic                 outputs
  onex_set_evaluators("default",   evaluate_edit_rule, evaluate_default, 0);
  onex_set_evaluators("editable",  evaluate_edit_rule, 0);
  onex_set_evaluators("clock",     evaluate_edit_rule, evaluate_clock_sync, evaluate_clock, 0);
  onex_set_evaluators("device",                        evaluate_device_logic, 0);
  onex_set_evaluators("user",                          evaluate_user, 0);
  onex_set_evaluators("notes",     evaluate_edit_rule, 0);
  onex_set_evaluators("battery",             evaluate_battery_in, 0);
  onex_set_evaluators("touch",               evaluate_touch_in, 0);
#if defined(HAS_MOTION)
  onex_set_evaluators("motion",              evaluate_motion_in, 0);
#endif
  onex_set_evaluators("button",              evaluate_button_in, 0);
  onex_set_evaluators("about",               evaluate_about_in, 0);
  onex_set_evaluators("backlight", evaluate_edit_rule, evaluate_light_logic, evaluate_backlight_out, 0);

  object_set_evaluator(onex_device_object, "device");

  object* battery;
  object* touch;
#if defined(HAS_MOTION)
  object* motion;
#endif
  object* button;
  object* backlight;
  object* oclock;
  object* watchface;
  object* home;
  object* inventory;
  object* watch;
  object* note1;
  object* note2;
  object* notes;
  object* about;

  char* responsesuid;
  char* deviceuid;
#if defined(HAS_MOTION)
  char* motionuid;
#endif
  char* backlightuid;
  char* watchfaceuid;
  char* watchuid;
  char* note1uid;
  char* note2uid;
  char* notesuid;

  user     =object_new(0, "user",      "user", 8);
  responses=object_new(0, "default",   "user responses", 12);
  battery  =object_new(0, "battery",   "battery", 4);
  touch    =object_new(0, "touch",     "touch", 6);
#if defined(HAS_MOTION)
  motion   =object_new(0, "motion",    "motion", 8);
#endif
  button   =object_new(0, "button",    "button", 4);
  backlight=object_new(0, "backlight", "light editable", 9);
  oclock   =object_new(0, "clock",     "clock event", 12);
  watchface=object_new(0, "editable",  "watchface editable", 6);
  home     =object_new(0, "editable",  "list editable", 4);
  inventory=object_new(0, "editable",  "list editable", 4);
  watch    =object_new(0, "default",   "watch", 4);
  note1    =object_new(0, "notes",     "text editable", 4);
  note2    =object_new(0, "notes",     "text editable", 4);
  notes    =object_new(0, "notes",     "text list editable", 4);
  about    =object_new(0, "about",     "about", 4);

  deviceuid   =object_property(onex_device_object, "UID");
  useruid     =object_property(user, "UID");
  responsesuid=object_property(responses, "UID");
  batteryuid  =object_property(battery, "UID");
  touchuid    =object_property(touch, "UID");
#if defined(HAS_MOTION)
  motionuid   =object_property(motion, "UID");
#endif
  buttonuid   =object_property(button, "UID");
  backlightuid=object_property(backlight, "UID");
  clockuid    =object_property(oclock, "UID");
  watchfaceuid=object_property(watchface, "UID");
  homeuid     =object_property(home, "UID");
  inventoryuid=object_property(inventory, "UID");
  watchuid    =object_property(watch, "UID");
  note1uid    =object_property(note1, "UID");
  note2uid    =object_property(note2, "UID");
  notesuid    =object_property(notes, "UID");
  aboutuid    =object_property(about, "UID");

  object_property_set(user, "responses", responsesuid);
  object_property_set(user, "inventory", inventoryuid);

  object_property_set(backlight, "light", "on");
  object_property_set(backlight, "level", "high");
  object_property_set(backlight, "timeout", "10000");
  object_property_set(backlight, "touch", touchuid);
#if defined(HAS_MOTION)
  object_property_set(backlight, "motion", motionuid);
#endif
  object_property_set(backlight, "button", buttonuid);

  object_property_set(oclock, "title", "OnexOS Clock");
  object_property_set(oclock, "ts", "%unknown");
  object_property_set(oclock, "tz", "%unknown");
  object_property_set(oclock, "device", deviceuid);

  object_property_set(watchface, "clock", clockuid);
  object_property_set(watchface, "ampm-24hr", "ampm");

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

  object_property_add(home, "list", inventoryuid);
  object_property_add(home, "list", notesuid);
  object_property_add(home, "list", aboutuid);
  object_property_add(home, "list", watchuid);
  object_property_add(home, "list", clockuid);
  object_property_add(home, "list", batteryuid);
//object_property_add(home, "list", note1uid);
//object_property_add(home, "list", note2uid);
//object_property_add(home, "list", homeuid);
//object_property_add(home, "list", useruid);
//object_property_add(home, "list", watchfaceuid);
//object_property_add(home, "list", backlightuid);
//object_property_add(home, "list", touchuid);
//object_property_add(home, "list", buttonuid);
#if defined(HAS_MOTION)
//object_property_add(home, "list", motionuid);
#endif

  object_property_set(watch, "battery",   batteryuid);
  object_property_set(watch, "watchface", watchfaceuid);

  object_property_set(user, "viewing", watchuid);

  object_property_add(onex_device_object, "user", useruid);
  object_property_add(onex_device_object, "io",   batteryuid);
  object_property_add(onex_device_object, "io",   touchuid);
#if defined(HAS_MOTION)
  object_property_add(onex_device_object, "io",   motionuid);
#endif
  object_property_add(onex_device_object, "io",   buttonuid);
  object_property_add(onex_device_object, "io",   backlightuid);
  object_property_add(onex_device_object, "io",   clockuid);

  onex_run_evaluators(batteryuid, 0);
  onex_run_evaluators(clockuid, 0);
  onex_run_evaluators(backlightuid, 0);
  onex_run_evaluators(aboutuid, 0);
}

int main() {

  boot_init();
  log_init();
  time_init_set(4+(unsigned long)&__BUILD_TIMESTAMP);
  gpio_init();

  set_up_gpio();

  touch_init(touched);
#if defined(HAS_MOTION)
  motion_init(moved);
#endif

  g2d_init();

  init_onex();

  time_ticker(every_second,  1000);
  time_ticker(every_10s,    10000);

  static uint8_t run_user_eval=1;

  while(1){

    if(run_user_eval){
      run_user_eval--;
      onex_run_evaluators(useruid, 0);
    }

    // --------------------

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

    if(user_active && button_pending){
      run_user_eval++;
    }

    // --------------------

    if(user_active && touch_pending){

      g2d_node_touch_event(touch_down, touch_info.x, touch_info.y);

      onex_run_evaluators(useruid, (void*)1);
      if(!touch_down) run_user_eval++;

      touch_events_seen++;
      touch_pending=false;
    }
  }
}

