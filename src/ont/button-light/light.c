
#include <time.h>
#include <stdlib.h>
#if defined(NRF5)
#include <boards.h>
#include <onex-kernel/gpio.h>
#if defined(HAS_SERIAL)
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

char* lightuid;
char* clockuid;

bool evaluate_light_io(object* light, void* d);

static void every_second()
{
  onex_run_evaluators(clockuid, 0);
}

int main()
{
  log_init();
  time_init();
#if defined(NRF5)
  gpio_init();
#if defined(HAS_SERIAL)
  serial_init(0,0);
#endif
#endif
  onex_init("light.db");

#if defined(BOARD_PCA10059)
  gpio_mode(LED1_G, OUTPUT);
  gpio_mode(LED2_B, OUTPUT);
#else
  log_write("\n------Starting Light Test-----\n");
#endif

  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);
  onex_set_evaluators("evaluate_light",  evaluate_edit_rule, evaluate_light_logic, evaluate_light_io, 0);
#if defined(SYNC_TO_PEER_CLOCK)
  onex_set_evaluators("evaluate_clock",  evaluate_clock_sync, evaluate_clock, 0);
#else
  onex_set_evaluators("evaluate_clock",  evaluate_clock, 0);
#endif

  object_set_evaluator(onex_device_object, (char*)"evaluate_device");
  char* deviceuid=object_property(onex_device_object, "UID");

  object* light=object_new(0, "evaluate_light", "editable light", 4);
  object_property_set(light, "light", "off");
  object_property_set(light, "device", deviceuid);
  lightuid=object_property(light, "UID");

  object* oclock=object_new(0, "evaluate_clock", "clock event", 12);
  object_set_persist(oclock, "none");
  object_property_set(oclock, "title", "OnexOS Light Clock");
  object_property_set(oclock, "ts", "%%unknown");
#if defined(SYNC_TO_PEER_CLOCK)
  object_property_set(oclock, "device", deviceuid);
#endif
  clockuid =object_property(oclock, "UID");

  object_property_add(onex_device_object, (char*)"io", lightuid);
  object_property_add(onex_device_object, (char*)"io", clockuid);

  onex_run_evaluators(lightuid, 0);
  time_ticker(every_second, 1000);

#if defined(BOARD_PCA10059)
  gpio_set(LED1_G,  LEDS_ACTIVE_STATE);
  gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#endif
  while(1) onex_loop();
}

bool evaluate_light_io(object* light, void* d)
{
#if defined(NRF5)
  if(object_property_is(light, "light", "on")){
#if defined(BOARD_PCA10059)
    gpio_set(LED2_B, LEDS_ACTIVE_STATE);
#endif
  } else {
#if defined(BOARD_PCA10059)
    gpio_set(LED2_B, !LEDS_ACTIVE_STATE);
#endif
  }
#else
  log_write("evaluate_light_io changed: "); object_log(light);
#endif
  return true;
}

