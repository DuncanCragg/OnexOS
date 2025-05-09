
#include <boards.h>
#include <onex-kernel/log.h>
#include <onex-kernel/boot.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/serial.h>
#if defined(BOARD_FEATHER_SENSE)
#include <onex-kernel/led-matrix.h>
#include <evaluators.h>
#endif
#include <onn.h>
#include <onr.h>
#include <ont.h>

object* button;
object* light;
#if defined(BOARD_FEATHER_SENSE)
object* ledmx;
object* battery;
#endif

static char* deviceuid;
static char* buttonuid;
static char* lightuid;
#if defined(BOARD_FEATHER_SENSE)
static char* ledmxuid;
static char* batteryuid;

static void every_10s(void*){
  onex_run_evaluators(batteryuid, 0);
}
#endif

static void button_changed(uint8_t pin, uint8_t type);

bool evaluate_button_io(object* button, void* pressed);
bool evaluate_light_io(object* light, void* d);
#if defined(BOARD_FEATHER_SENSE)
bool evaluate_ledmx_io(object* ledmx, void* d);
// REVISIT: evaluators and behaviours sept
#endif

int main(){ // REVISIT: needs to be in OK and call up here like ont-vk

  properties* config = properties_new(32);
#if defined(BOARD_PCA10059)
  properties_set(config, "channels", list_new_from("radio serial",2));
#elif defined(BOARD_FEATHER_SENSE)
  properties_set(config, "channels", list_new_from("radio",2));
#define TEST_MODE
#ifdef  TEST_MODE
  properties_set(config, "flags", list_new_from("debug-on-serial log-to-led",2));
  properties_set(config, "test-uid-prefix", value_new("iot"));
#endif
#endif

  time_init();

  log_init(config); // has: "if(debug_on_serial) serial_init(...);"
  if(debug_on_serial) serial_ready_state(); // blocks until serial stable

  random_init();

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
  led_matrix_fill_rgb((led_matrix_rgb){0, 16, 0});
  led_matrix_show();
#define ADC_CHANNEL 0
  gpio_adc_init(BATTERY_V, ADC_CHANNEL);
#endif

  if(debug_on_serial){
    if(serial_ready_state() == SERIAL_POWERED_NOT_READY){
#if defined(BOARD_FEATHER_SENSE)
      led_matrix_fill_col("#100");
      led_matrix_show();
#endif
      log_flash(1,0,0);
      time_delay_ms(400);
      log_flash(1,0,0);

      boot_reset(false);
    }
  }

  // ------------------------------

  log_write("Starting Onex.....\n");

  onex_init(config);

  onex_set_evaluators("evaluate_button", evaluate_edit_rule, evaluate_button_io, 0);
  onex_set_evaluators("evaluate_light",  evaluate_edit_rule, evaluate_light_logic, evaluate_light_io, 0);
#if defined(BOARD_FEATHER_SENSE)
  onex_set_evaluators("evaluate_ledmx",   evaluate_edit_rule, evaluate_light_logic, evaluate_ledmx_io, 0);
  onex_set_evaluators("evaluate_battery", evaluate_battery_in, 0);
#endif
  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);

  button=object_new(0, "evaluate_button", "editable button", 4);
  light =object_new(0, "evaluate_light",  "editable light", 4);
#if defined(BOARD_FEATHER_SENSE)
  ledmx  =object_new(0, "evaluate_ledmx",   "editable light", 4);
  battery=object_new(0, "evaluate_battery", "battery", 4);
#endif

  deviceuid=object_property(onex_device_object, "UID");
  buttonuid=object_property(button,"UID");
  lightuid =object_property(light, "UID");
#if defined(BOARD_FEATHER_SENSE)
  ledmxuid   =object_property(ledmx, "UID");
  batteryuid =object_property(battery, "UID");
#endif

  object_property_set(button, "name", "mango");

  object_property_set(light, "light", "off");
#if defined(BOARD_FEATHER_SENSE)
  object_property_set(ledmx, "light", "on");
  object_property_set(ledmx, "colour", "#030303");
#endif

  object_set_evaluator(onex_device_object, "evaluate_device");
  object_property_set(onex_device_object, "name", "IoT");
  object_property_add(onex_device_object, "io", buttonuid);
  object_property_add(onex_device_object, "io", lightuid);
#if defined(BOARD_FEATHER_SENSE)
  object_property_add(onex_device_object, "io", ledmxuid);
  object_property_add(onex_device_object, "io", batteryuid);
#endif

  onex_run_evaluators(lightuid, 0);
#if defined(BOARD_FEATHER_SENSE)
  onex_run_evaluators(ledmxuid, 0);
  onex_run_evaluators(batteryuid, 0);
  time_ticker(every_10s, 0, 10000);
#endif

  while(1){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
}

static volatile bool button_pressed=false;

static void button_changed(uint8_t pin, uint8_t type){
  button_pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  onex_run_evaluators(buttonuid, (void*)button_pressed);
}

bool evaluate_button_io(object* button, void* pressed) {
  char* s=(pressed? "down": "up");
  object_property_set(button, "state", s);
  return true;
}

bool evaluate_light_io(object* light, void* d) {

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
bool evaluate_ledmx_io(object* ledmx, void* d) {
  if(object_property_is(ledmx, "light", "on")){
    led_matrix_fill_col(object_property(ledmx, "colour"));
    led_matrix_show();
  } else {
    led_matrix_fill_rgb((led_matrix_rgb){0, 0, 0});
    led_matrix_show();
  }
  return true;
}
#endif
















