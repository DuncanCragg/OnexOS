
#include <time.h>
#include <stdlib.h>
#if defined(NRF5)
#include <boards.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/serial.h>
#else
#include <onex-kernel/config.h>
#endif
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onn.h>
#include <onr.h>
#include <ont.h>

#if defined(NRF5)
#define SYNC_TO_PEER_CLOCK
#endif

char* buttonuid;
char* clockuid;

static void every_second(void*){
  onex_run_evaluators(clockuid, 0);
}

static volatile bool button_pressed=false;

#if defined(NRF5)
static void button_changed(uint8_t pin, uint8_t type){
  button_pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  onex_run_evaluators(buttonuid, (void*)button_pressed);
}
#else
static void every_tick(void*){
  button_pressed=!button_pressed;
  onex_run_evaluators(buttonuid, (void*)button_pressed);
}
#endif

bool evaluate_button_io(object* button, void* pressed);

int main(int argc, char *argv[]) {

#if defined(NRF5)
  properties* config = properties_new(32);
  properties_set(config, "channels", list_new_from_fixed("serial"));
//properties_set(config, "flags", list_new_from_fixed("debug-on-serial log-to-led"));
  properties_set(config, "test-uid-prefix", value_new("button"));
#else
  properties* config = get_config(argc, argv, "button", "log-onp");
  if(!config) return -1;
#endif

  time_init();
  log_init(config);
#if defined(NRF5)
  gpio_init();
#endif

  onex_init(config);

#if defined(BOARD_PCA10059)
  gpio_mode_cb(BUTTON_1, INPUT_PULLUP, RISING_AND_FALLING, button_changed);
#else
  log_write("\n------Starting Button Test-----\n");
#endif

  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);
  onex_set_evaluators("evaluate_button", evaluate_edit_rule, evaluate_button_io, 0);
#if defined(SYNC_TO_PEER_CLOCK)
  onex_set_evaluators("evaluate_clock",  evaluate_clock_sync, evaluate_clock, 0);
#else
  onex_set_evaluators("evaluate_clock",  evaluate_clock, 0);
#endif

  object_set_evaluator(onex_device_object, "evaluate_device");
#if defined(SYNC_TO_PEER_CLOCK)
  char* deviceuid=object_property(onex_device_object, "UID");
#endif

  object* button=object_new("uid-button", "evaluate_button", "editable button", 4);
  object_property_set(button, "name", "£€§");
  object_property_set(button, "state", "up");
  buttonuid=object_property(button, "UID");

  object* oclock=object_new("uid-button-clock", "evaluate_clock", "clock event", 12);
  object_set_persist(oclock, "none");
  object_property_set(oclock, "title", "OnexOS Button Clock");
  object_property_set(oclock, "ts", "%%unknown");
#if defined(SYNC_TO_PEER_CLOCK)
  object_property_set(oclock, "device", deviceuid);
#endif
  clockuid =object_property(oclock, "UID");

  object_property_add(onex_device_object, "io", buttonuid);
  object_property_add(onex_device_object, "io", clockuid);

  time_ticker(every_second, 0, 1000);
#if !defined(NRF5)
  time_ticker(every_tick, 0, 2000);
#endif

  while(1){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
}

bool evaluate_button_io(object* button, void* pressed) {
  char* s=(pressed? "down": "up");
  object_property_set(button, "state", s);
#if !defined(NRF5)
  log_write("evaluate_button_io: "); object_log(button);
#endif
  return true;
}


