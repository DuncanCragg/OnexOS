
#include <stdbool.h>
#include <stdint.h>

#include <boards.h>

#include <onex-kernel/boot.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/compass.h>
#include <onex-kernel/seesaw.h>
#include <onex-kernel/display.h>
#include <onex-kernel/touch.h>

#include <onn.h>
#include <onr.h>

#if defined(BOARD_MAGIC3)
#include <g2d.h>
#endif

extern char __BUILD_TIME;

bool display_on=true;

extern char* useruid;

// ------------------- evaluators ----------------

#define ROTARY_ENC_ADDRESS 0x36
#define ROTARY_ENC_BUTTON  24

#define BATT_ADC_CHANNEL 0
#define POT1_ADC_CHANNEL 1
#define POT2_ADC_CHANNEL 2

static bool do_rotary_encoder=true;

void evaluators_init(){

  gpio_adc_init(BATTERY_V, BATT_ADC_CHANNEL);

#if !defined(BOARD_MAGIC3)
  compass_init();

  time_delay_ms(50); // seesaw needs a minute to get its head straight
  uint16_t version_hi = seesaw_status_version_hi(ROTARY_ENC_ADDRESS);
  if(version_hi != 4991){
    log_write("rotary encoder id 4991 not found: %d\n", version_hi);
    do_rotary_encoder = false;
  }
  else{
    log_write("rotary encoder found, doing rotaries\n");
    gpio_adc_init(GPIO_A0, POT1_ADC_CHANNEL);
    gpio_adc_init(GPIO_A1, POT2_ADC_CHANNEL);
    seesaw_gpio_input_pullup(ROTARY_ENC_ADDRESS, ROTARY_ENC_BUTTON);
  }
#endif
}

bool evaluate_default(object* obj, void* d) {
  log_write("evaluate_default d=%p\n", d);
//object_log(obj);
  return true;
}

#define BATTERY_ZERO_PERCENT 3400
#define BATTERY_100_PERCENT 4100
#define BATTERY_PERCENT_STEPS 2
bool evaluate_battery_in(object* bat, void* d) {

  int32_t bv = gpio_read(BATT_ADC_CHANNEL);
  int32_t mv = bv*2000/(1024/(33/10));
  int16_t pc = ((mv-BATTERY_ZERO_PERCENT)
                 *100
                 / ((BATTERY_100_PERCENT-BATTERY_ZERO_PERCENT)*BATTERY_PERCENT_STEPS)
               ) * BATTERY_PERCENT_STEPS;
  if(pc<0) pc=0;
  if(pc>100) pc=100;

  object_property_set_fmt(bat, "percent", "%d%% %ldmv", pc, mv);

#if defined(BOARD_MAGIC3)
  uint8_t batt=gpio_get(CHARGE_SENSE);
  object_property_set(bat, "status", batt? "powering": "charging");
#endif

  return true;
}

#if !defined(BOARD_MAGIC3)
bool evaluate_compass_in(object* compass, void* d){
  compass_info_t ci = compass_direction();
  object_property_set_fmt(compass, "direction", "%d°", ci.o);
  return true;
}

bool evaluate_bcs_in(object* bcs, void* d){

  if(!do_rotary_encoder){
    object_property_set(bcs, "brightness", "255");
    object_property_set(bcs, "colour",      "85"); // green
    object_property_set(bcs, "softness",     "0");
    return true;
  }

  int32_t rot_pos      = seesaw_encoder_position(ROTARY_ENC_ADDRESS);
  bool    rot_pressed = !seesaw_gpio_read(ROTARY_ENC_ADDRESS, ROTARY_ENC_BUTTON);

  int16_t pot1 = gpio_read(POT1_ADC_CHANNEL);
  int16_t pot2 = gpio_read(POT2_ADC_CHANNEL);

  if(pot1<0) pot1=0;
  if(pot2<0) pot2=0;

  uint8_t brightness = pot1/4;                 // 0..1023
  uint8_t colour     = (uint8_t)(rot_pos * 4); // lo byte, 4 lsb per click
  uint8_t softness   = 255-pot2/4;             // 0..1023

  object_property_set_fmt(bcs, "brightness", "%d", brightness);
  object_property_set_fmt(bcs, "colour",     "%d", colour);
  object_property_set_fmt(bcs, "softness",   "%d", softness);

  return true;
}
#endif

bool evaluate_button_in(object* btn, void* d) {
  bool button_pressed = !!d;
  object_property_set(btn, "state", button_pressed? "down": "up");
  return true;
}

#if defined(BOARD_MAGIC3)
bool evaluate_touch_in(object* tch, void* d) {

  touch_info_t* touch_info=(touch_info_t*)d;
  object_property_set_fmt(tch, "coords", "%3d %3d", touch_info->x, touch_info->y);
  object_property_set(    tch, "action",            touch_actions[touch_info->action]);

  return true;
}

bool evaluate_about_in(object* abt, void* d) {

  object_property_set_fmt(abt, "build-info", "%lu", (unsigned long)&__BUILD_TIME);
  object_property_set_fmt(abt, "cpu",        "%d%%", boot_cpu());

  return true;
}

#define DISPLAY_SLEEP_HARD true

bool evaluate_backlight_out(object* blt, void* d) {

  bool light_on=object_property_is(blt, "light", "on");

  if(light_on && !display_on){

    display_fast_wake(DISPLAY_SLEEP_HARD);

    bool mid =object_property_is(blt, "level", "mid");
    bool high=object_property_is(blt, "level", "high");
    gpio_set(LCD_BACKLIGHT, (mid||high)? LEDS_ACTIVE_STATE: !LEDS_ACTIVE_STATE);

    touch_wake();

    display_on=true;

    onex_run_evaluators(useruid, 0);
  }
  else
  if(!light_on && display_on){

    display_on=false;

    touch_sleep();

    gpio_set(LCD_BACKLIGHT, !LEDS_ACTIVE_STATE);

    g2d_clear_screen(0x00);
    g2d_render();

    display_fast_sleep(DISPLAY_SLEEP_HARD);
  }
  return true;
}
#endif

#if defined(HAS_MOTION)
bool evaluate_motion_in(object* mtn, void* d) {

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

  object_property_set_fmt(mtn, "x-y-z-m", "%d %d %d %d",
                          motion_info.x, motion_info.y, motion_info.z, motion_info.m);
  object_property_set(mtn, "gesture", viewscreen? "view-screen": "none");

  return true;
}
#endif

