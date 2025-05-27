
#include <boards.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/log.h>
#include <onex-kernel/boot.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/serial.h>
#if defined(BOARD_FEATHER_SENSE)
#include <onex-kernel/colours.h>
#include <onex-kernel/led-matrix.h>
#include <onex-kernel/led-strip.h>
#include <io-evaluators.h>
#endif
#include <onn.h>
#include <onr.h>
#include <ont.h>

object* button;
#if defined(BOARD_FEATHER_SENSE)
object* battery;
object* bcs; // Brightness/Colour/Softness (HSV)
object* compass;
#endif
object* light;
#if defined(BOARD_FEATHER_SENSE)
object* ledmx;
#endif

static char* deviceuid;
static char* buttonuid;
#if defined(BOARD_FEATHER_SENSE)
static char* batteryuid;
static char* bcsuid;
static char* compassuid;
#endif
static char* lightuid;
#if defined(BOARD_FEATHER_SENSE)
static char* ledmxuid;
#endif

#if defined(BOARD_FEATHER_SENSE)
static void poll_input_evaluators(void*){
  onex_run_evaluators(batteryuid, 0);
  onex_run_evaluators(bcsuid, 0);
  onex_run_evaluators(compassuid, 0);
}
#endif

static void button_changed(uint8_t pin, uint8_t type);

static bool evaluate_light_out(object* light, void* d);
#if defined(BOARD_FEATHER_SENSE)
static bool evaluate_ledmx_out(object* ledmx, void* d);
// REVISIT: evaluators and behaviours sept
#endif

void set_up_gpio(){
  gpio_init();
#if defined(BOARD_PCA10059)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
  gpio_mode(LED1_G, OUTPUT);
  gpio_mode(LED2_B, OUTPUT);
  gpio_set(LED1_G, LEDS_ACTIVE_STATE);
  gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_ITSYBITSY)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
  gpio_mode(LED_1, OUTPUT);
  gpio_set(LED_1, LEDS_ACTIVE_STATE);
#elif defined(BOARD_FEATHER_SENSE)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
  gpio_mode(LED_1, OUTPUT);
  gpio_set(LED_1, !LEDS_ACTIVE_STATE);

  led_matrix_init();
  led_strip_init();
  led_matrix_fill_rgb((colours_rgb){0, 16, 0});
  led_strip_fill_rgb((colours_rgb){0, 16, 0});
  led_matrix_show();
  led_strip_show();
#endif
}

int main(){ // REVISIT: needs to be in OK and call up here like ont-vk

  properties* config = properties_new(32);
#if defined(BOARD_PCA10059)
  properties_set(config, "channels", list_new_from_fixed("radio serial"));
#elif defined(BOARD_FEATHER_SENSE)
  properties_set(config, "channels", list_new_from_fixed("radio"));
#define TEST_MODE
#ifdef  TEST_MODE
  properties_set(config, "flags", list_new_from_fixed("debug-on-serial log-to-led log-onp"));
  properties_set(config, "test-uid-prefix", value_new("iot"));
#endif
#endif

  boot_init();

  time_init();
  random_init();

  log_init(config); // has: "if(debug_on_serial) serial_init(...);"
  if(debug_on_serial) serial_ready_state(); // blocks until serial stable
  // REVISIT: serial_loop() in early stages

  set_up_gpio();

  if(debug_on_serial){
    if(serial_ready_state() == SERIAL_POWERED_NOT_READY){
#if defined(BOARD_FEATHER_SENSE)
      led_strip_fill_col("#700");
      led_matrix_fill_col("#700");
      led_matrix_show(); led_strip_show();
#endif
      log_flash(1,0,0);
      time_delay_ms(500);
      boot_reset(false);
    }
  }

  // ------------------------------

  log_write("Starting Onex.....\n");

  evaluators_init();

  onex_init(config);

  onex_set_evaluators("evaluate_button", evaluate_edit_rule, evaluate_button_in, 0);
#if defined(BOARD_FEATHER_SENSE)
  onex_set_evaluators("evaluate_battery", evaluate_battery_in, 0);
  onex_set_evaluators("evaluate_bcs",     evaluate_bcs_in, 0);
  onex_set_evaluators("evaluate_compass", evaluate_compass_in, 0);
#endif
  onex_set_evaluators("evaluate_light",  evaluate_edit_rule, evaluate_light_logic, evaluate_light_out, 0);
#if defined(BOARD_FEATHER_SENSE)
  onex_set_evaluators("evaluate_ledmx",   evaluate_edit_rule, evaluate_light_logic, evaluate_ledmx_out, 0);
#endif
  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);

  button =object_new(0, "evaluate_button",  "editable button", 4);
#if defined(BOARD_FEATHER_SENSE)
  battery=object_new(0, "evaluate_battery", "battery", 4);
  bcs    =object_new(0, "evaluate_bcs",     "bcs", 5);
  compass=object_new(0, "evaluate_compass", "compass", 4);
#endif
  light  =object_new(0, "evaluate_light",   "editable light", 8);
#if defined(BOARD_FEATHER_SENSE)
  ledmx  =object_new(0, "evaluate_ledmx",   "editable light", 8);
#endif

  deviceuid=object_property(onex_device_object, "UID");
  buttonuid=object_property(button,"UID");
#if defined(BOARD_FEATHER_SENSE)
  batteryuid =object_property(battery, "UID");
  bcsuid     =object_property(bcs, "UID");
  compassuid =object_property(compass, "UID");
#endif
  lightuid =object_property(light, "UID");
#if defined(BOARD_FEATHER_SENSE)
  ledmxuid   =object_property(ledmx, "UID");
#endif

  object_property_set(button, "name", "mango");

  object_property_set(light, "light", "off");
#if defined(BOARD_FEATHER_SENSE)
  object_property_set(ledmx, "light", "on");
  object_property_set(ledmx, "colour", "%0300ff");
//object_property_set(ledmx, "device", deviceuid);
#endif

  object_set_evaluator(onex_device_object, "evaluate_device");
  object_property_set(onex_device_object, "name", "IoT");
  object_property_add(onex_device_object, "io", buttonuid);
#if defined(BOARD_FEATHER_SENSE)
  object_property_add(onex_device_object, "io", batteryuid);
  object_property_add(onex_device_object, "io", compassuid);
  object_property_add(onex_device_object, "io", bcsuid);
#endif
  object_property_add(onex_device_object, "io", lightuid);
#if defined(BOARD_FEATHER_SENSE)
  object_property_add(onex_device_object, "io", ledmxuid);
#endif

  onex_run_evaluators(lightuid, 0);
#if defined(BOARD_FEATHER_SENSE)
  time_ticker(poll_input_evaluators, 0, 300);
  onex_run_evaluators(ledmxuid, 0);
#endif

  while(true){

    uint64_t ct=time_ms();

    if(!onex_loop()){
      time_delay_ms(5);
    }

    static uint64_t feeding_time=0;
    if(ct>feeding_time){
      boot_feed_watchdog();
      feeding_time=ct+1000;
    }
  }
}

static volatile bool button_pressed=false;

static void button_changed(uint8_t pin, uint8_t type){
  button_pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  onex_run_evaluators(buttonuid, (void*)button_pressed);
}

bool evaluate_light_out(object* light, void* d) {

  if(object_property_is(light, "light", "on")){
#if defined(BOARD_PCA10059)
    gpio_set(LED2_B, LEDS_ACTIVE_STATE);
#elif defined(BOARD_ITSYBITSY)
    gpio_set(LED_1, LEDS_ACTIVE_STATE);
#elif defined(BOARD_FEATHER_SENSE)
    gpio_set(LED_1, LEDS_ACTIVE_STATE);
#endif
  } else {
#if defined(BOARD_PCA10059)
    gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_ITSYBITSY)
    gpio_set(LED_1, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_FEATHER_SENSE)
    gpio_set(LED_1, !LEDS_ACTIVE_STATE);
#endif
  }
  return true;
}

#if defined(BOARD_FEATHER_SENSE)
bool evaluate_ledmx_out(object* ledmx, void* d) {
  if(object_property_is(ledmx, "light", "on")){
    char* col = object_property(ledmx, "colour");
    led_matrix_fill_col(col);
    led_strip_fill_col(col);
    led_matrix_show();
    led_strip_show();
  } else {
    led_matrix_fill_rgb((colours_rgb){0, 0, 0});
    led_strip_fill_rgb((colours_rgb){0, 0, 0});
    led_matrix_show();
    led_strip_show();
  }
  return true;
}
#endif
















