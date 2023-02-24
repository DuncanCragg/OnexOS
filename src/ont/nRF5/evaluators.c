
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

extern char __BUILD_TIME;
extern char __BOOTLOADER_NUMBER;

#define ADC_CHANNEL 0

extern bool user_active;
extern char* useruid;
extern touch_info_t touch_info;

// ------------------- evaluators ----------------

// XXX separated out to remind me to extend the respective APIs to allow sprintf-style
// XXX varargs for path segments, value construction and g2d_node_text()
static char valuebuf[64];

bool evaluate_default(object* obj, void* d) {
  log_write("evaluate_default d=%p\n", d);
//object_log(obj);
  return true;
}

#define BATTERY_ZERO_PERCENT 3400
#define BATTERY_100_PERCENT 4000
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
  snprintf(valuebuf, 64, "%d%%(%ld)", pc, mv);

  object_property_set(bat, "percent", valuebuf);

  uint8_t batt=gpio_get(CHARGE_SENSE);
  snprintf(valuebuf, 64, "%s", batt? "powering": "charging");
  object_property_set(bat, "status", valuebuf);

  return true;
}

bool evaluate_touch_in(object* tch, void* d) {

  snprintf(valuebuf, 64, "%3d %3d", touch_info.x, touch_info.y);
  object_property_set(tch, "coords", valuebuf);

  snprintf(valuebuf, 64, "%s", touch_actions[touch_info.action]);
  object_property_set(tch, "action", valuebuf);

#if defined(DO_LATER)
  snprintf(valuebuf, 64, "%d", touch_info_stroke);
  object_property_set(tch, "stroke", valuebuf);
#endif

  return true;
}

#if defined(DO_LATER)
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

  snprintf(valuebuf, 64, "%d %d %d %d",
                          motion_info.x, motion_info.y, motion_info.z, motion_info.m);
  object_property_set(mtn, "x-y-z-m", valuebuf);
  object_property_set(mtn, "gesture", viewscreen? "view-screen": "none");

  return true;
}
#endif

bool evaluate_button_in(object* btn, void* d) {
  bool button_pressed=(gpio_get(BUTTON_1)==BUTTONS_ACTIVE_STATE);
  object_property_set(btn, "state", button_pressed? "down": "up");
  return true;
}

bool evaluate_about_in(object* abt, void* d) {
  snprintf(valuebuf, 64, "%lu %lu",
                          (unsigned long)&__BUILD_TIME,
                          (unsigned long)&__BOOTLOADER_NUMBER);

  object_property_set(abt, "build-info", valuebuf);

  snprintf(valuebuf, 64, "%d%%", boot_cpu());
  object_property_set(abt, "cpu", valuebuf);

  return true;
}

bool evaluate_backlight_out(object* blt, void* d) {

  bool light_on=object_property_is(blt, "light", "on");

  if(light_on && !user_active){

    display_fast_wake();

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

    display_fast_sleep();
  }
  return true;
}

