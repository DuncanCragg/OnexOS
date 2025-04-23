
#include <boards.h>
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/serial.h>
#if defined(BOARD_FEATHER_SENSE)
#include <onex-kernel/led-matrix.h>
#endif
#include <onn.h>
#include <onr.h>
#include <ont.h>

object* button;
object* light;
#if defined(BOARD_FEATHER_SENSE)
object* ledmx;
#endif

char* deviceuid;
char* buttonuid;
char* lightuid;
#if defined(BOARD_FEATHER_SENSE)
char* ledmxuid;
#endif

static void button_changed(uint8_t pin, uint8_t type);

bool evaluate_button_io(object* button, void* pressed);
bool evaluate_light_io(object* light, void* d);
#if defined(BOARD_FEATHER_SENSE)
bool evaluate_ledmx_io(object* ledmx, void* d);
#endif

void* x;
#define WHERESTHEHEAP(s) x = malloc(1); log_write("heap after %s: %x\n", s, x);

int main(){ // REVISIT: needs to be in OK and call up here like ont-vk

  properties* config = properties_new(32);
#if defined(BOARD_PCA10059)
  properties_set(config, "channels", list_new_from("radio serial",2));
#elif defined(BOARD_FEATHER_SENSE)
  properties_set(config, "channels", list_new_from("radio",2));
  properties_set(config, "flags", list_new_from("log-to-serial",2));
#endif

  time_init();
  log_init(config);
  random_init();
  gpio_init();

  onex_init(config);

#if defined(BOARD_PCA10059)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
  gpio_mode(LED1_G, OUTPUT);
  gpio_mode(LED2_B, OUTPUT);
#elif defined(BOARD_ITSYBITSY)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
  gpio_mode(LED_1, OUTPUT);
#elif defined(BOARD_FEATHER_SENSE)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
  gpio_mode(LED_1, OUTPUT);
  led_matrix_init();
#endif

  onex_set_evaluators("evaluate_button", evaluate_edit_rule, evaluate_button_io, 0);
  onex_set_evaluators("evaluate_light",  evaluate_edit_rule, evaluate_light_logic, evaluate_light_io, 0);
#if defined(BOARD_FEATHER_SENSE)
  onex_set_evaluators("evaluate_ledmx",  evaluate_edit_rule, evaluate_light_logic, evaluate_ledmx_io, 0);
#endif
  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);

  button=object_new(0, "evaluate_button", "editable button", 4);
  light =object_new(0, "evaluate_light",  "editable light", 4);

#if defined(BOARD_FEATHER_SENSE)
  ledmx =object_new(0, "evaluate_ledmx",  "editable light", 4);
#endif

  deviceuid=object_property(onex_device_object, "UID");
  buttonuid=object_property(button,"UID");
  lightuid =object_property(light, "UID");
#if defined(BOARD_FEATHER_SENSE)
  ledmxuid =object_property(ledmx, "UID");
#endif

  object_property_set(button, "name", "mango");

  object_property_set(light, "light", "off");

#if defined(BOARD_FEATHER_SENSE)
  object_property_set(ledmx, "light", "off");
  object_property_set(ledmx, "colour", "#010");
#endif

  object_set_evaluator(onex_device_object, "evaluate_device");
  object_property_add(onex_device_object, "io", buttonuid);
  object_property_add(onex_device_object, "io", lightuid);
#if defined(BOARD_FEATHER_SENSE)
  object_property_add(onex_device_object, "io", ledmxuid);
#endif

  onex_run_evaluators(lightuid, 0);
#if defined(BOARD_FEATHER_SENSE)
  onex_run_evaluators(ledmxuid, 0);
#endif

#if defined(BOARD_PCA10059)
  gpio_set(LED1_G, LEDS_ACTIVE_STATE);
  gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_ITSYBITSY)
  gpio_set(LED_1, LEDS_ACTIVE_STATE);
#elif defined(BOARD_FEATHER_SENSE)
  gpio_set(LED_1, LEDS_ACTIVE_STATE);
  led_matrix_fill_rgb((led_matrix_rgb){0, 16, 0});
  led_matrix_show();
#endif

  while(1){
    onex_loop();
  }
}

static volatile bool button_pressed=false;

static void button_changed(uint8_t pin, uint8_t type){
  button_pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  onex_run_evaluators(buttonuid, (void*)button_pressed);
}

bool evaluate_button_io(object* button, void* pressed)
{
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
















