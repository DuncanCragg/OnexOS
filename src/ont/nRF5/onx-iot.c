
#include <boards.h>
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <onex-kernel/gpio.h>
#if defined(HAS_SERIAL)
#include <onex-kernel/serial.h>
#endif
#include <onn.h>
#include <onr.h>

object* button;
object* light;
char* buttonuid;
char* lightuid;

static void button_changed(uint8_t pin, uint8_t type);

bool evaluate_button_io(object* button, void* pressed);
bool evaluate_light_io(object* light, void* d);

void* x;
#define WHERESTHEHEAP(s) x = malloc(1); log_write("heap after %s: %x\n", s, x);

int main()
{
  log_init();
  time_init();
  gpio_init();
#if defined(HAS_SERIAL)
  serial_init(0,0);
#endif

  onex_init("");

#if defined(BOARD_PCA10059)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
  gpio_mode(LED1_G, OUTPUT);
  gpio_mode(LED2_B, OUTPUT);
#elif defined(BOARD_ITSYBITSY)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
  gpio_mode(LED_1, OUTPUT);
#endif

  onex_set_evaluators("evaluate_button", evaluate_edit_rule, evaluate_button_io, 0);
  onex_set_evaluators("evaluate_light",  evaluate_edit_rule, evaluate_light_logic, evaluate_light_io, 0);
  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);

  button=object_new(0, "evaluate_button", "editable button", 4);
  light =object_new(0, "evaluate_light",  "editable light", 4);
  buttonuid=object_property(button, "UID");
  lightuid=object_property(light, "UID");

  object_property_set(button, "name", "mango");

  object_property_set(light, "light", "off");

  object_set_evaluator(onex_device_object, (char*)"evaluate_device");
  object_property_add(onex_device_object, (char*)"io", buttonuid);
  object_property_add(onex_device_object, (char*)"io", lightuid);

  onex_run_evaluators(lightuid, 0);

#if defined(BOARD_PCA10059)
  gpio_set(LED1_G, LEDS_ACTIVE_STATE);
  gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_ITSYBITSY)
  gpio_set(LED_1, LEDS_ACTIVE_STATE);
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
  char* s=(char*)(pressed? "down": "up");
  object_property_set(button, "state", s);
  return true;
}

bool evaluate_light_io(object* light, void* d)
{
  if(object_property_is(light, "light", "on")){
#if defined(BOARD_PCA10059)
    gpio_set(LED2_B, LEDS_ACTIVE_STATE);
#elif defined(BOARD_ITSYBITSY)
    gpio_set(LED_1, LEDS_ACTIVE_STATE);
#endif
  } else {
#if defined(BOARD_PCA10059)
    gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#elif defined(BOARD_ITSYBITSY)
    gpio_set(LED_1, !LEDS_ACTIVE_STATE);
#endif
  }
  return true;
}

