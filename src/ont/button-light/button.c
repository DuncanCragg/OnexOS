
#include <time.h>
#include <stdlib.h>
#if defined(NRF5)
#include <boards.h>
#include <onex-kernel/gpio.h>
#if defined(LOG_TO_SERIAL)
#include <onex-kernel/serial.h>
#endif
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

static volatile bool button_pressed=false;

static void every_second()
{
  onex_run_evaluators(clockuid, 0);
}

#if defined(NRF5)
static void button_changed(uint8_t pin, uint8_t type){
  button_pressed=(gpio_get(pin)==BUTTONS_ACTIVE_STATE);
  onex_run_evaluators(buttonuid, (void*)button_pressed);
}
#endif

bool evaluate_button_io(object* button, void* pressed);

int main()
{
  log_init();
  time_init();
#if defined(NRF5)
  gpio_init();
#if defined(LOG_TO_SERIAL)
  serial_init(0,0);
#endif
#endif

  properties* config = properties_new(32);
#if defined(NRF5)
  properties_set(config, "channels", list_new_from("radio",1));
#else
  properties_set(config, "dbpath", value_new("button.db"));
  properties_set(config, "channels", list_new_from("ipv6", 1));
  properties_set(config, "ipv6_groups", list_new_from("ff12::1234",1));
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

  object_set_evaluator(onex_device_object, (char*)"evaluate_device");
#if defined(SYNC_TO_PEER_CLOCK)
  char* deviceuid=object_property(onex_device_object, "UID");
#endif

  object* button=object_new(0, "evaluate_button", "editable button", 4);
  object_property_set(button, "name", "£€§");
  buttonuid=object_property(button, "UID");

  object* oclock=object_new(0, "evaluate_clock", "clock event", 12);
  object_set_persist(oclock, "none");
  object_property_set(oclock, "title", "OnexOS Button Clock");
  object_property_set(oclock, "ts", "%%unknown");
#if defined(SYNC_TO_PEER_CLOCK)
  object_property_set(oclock, "device", deviceuid);
#endif
  clockuid =object_property(oclock, "UID");

  object_property_add(onex_device_object, (char*)"io", buttonuid);
  object_property_add(onex_device_object, (char*)"io", clockuid);

  time_ticker(every_second, 1000);

#if !defined(NRF5)
  uint32_t lasttime=0;
#endif

  while(1){

    onex_loop();

#if !defined(NRF5)
    if(time_ms() > lasttime+1200u){
      lasttime=time_ms();
      button_pressed=!button_pressed;
      onex_run_evaluators(buttonuid, (void*)button_pressed);
    }
#endif
  }
}

bool evaluate_button_io(object* button, void* pressed)
{
  char* s=(char*)(pressed? "down": "up");
  object_property_set(button, "state", s);
#if !defined(NRF5)
  log_write("evaluate_button_io: "); object_log(button);
#endif
  return true;
}


