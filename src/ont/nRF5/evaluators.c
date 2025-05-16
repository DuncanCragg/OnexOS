
#include <stdbool.h>
#include <stdint.h>

#include <boards.h>

#include <onex-kernel/boot.h>
#include <onex-kernel/log.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/display.h>
#include <onex-kernel/touch.h>

#include <onn.h>
#include <onr.h>

#if defined(BOARD_MAGIC3)
#include <g2d.h>
#endif

extern char __BUILD_TIME;

#define ADC_CHANNEL 0

bool user_active=true;

extern char* useruid;

// ------------------- evaluators ----------------

bool evaluate_default(object* obj, void* d) {
  log_write("evaluate_default d=%p\n", d);
//object_log(obj);
  return true;
}

#define BATTERY_ZERO_PERCENT 3400
#define BATTERY_100_PERCENT 4100
#define BATTERY_PERCENT_STEPS 2
bool evaluate_battery_in(object* bat, void* d) {

  int32_t bv = gpio_read(ADC_CHANNEL);
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

  if(light_on && !user_active){

    display_fast_wake(DISPLAY_SLEEP_HARD);

    bool mid =object_property_is(blt, "level", "mid");
    bool high=object_property_is(blt, "level", "high");
    gpio_set(LCD_BACKLIGHT, (mid||high)? LEDS_ACTIVE_STATE: !LEDS_ACTIVE_STATE);

    touch_wake();

    user_active=true;

    onex_run_evaluators(useruid, 0);
  }
  else
  if(!light_on && user_active){

    user_active=false;

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

