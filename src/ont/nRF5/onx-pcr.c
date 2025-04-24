
#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onn.h>
#include <onr.h>
#include <ont.h>

int main(int argc, char *argv[]) {

  properties* config = properties_new(32);
  properties_set(config, "channels", list_new_from("radio serial",2));
#define TEST_MODE
#ifdef  TEST_MODE
  properties_set(config, "flags", list_new_from("log-to-serial log-to-leds",2));
  properties_set(config, "test-uid-prefix", value_new("pcr"));
#endif

  time_init();
  log_init(config);
  random_init();

  onex_init(config);

  log_write("\n------Starting PCR-----\n");

  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);
  object_set_evaluator(onex_device_object, "evaluate_device");
  object_property_set(onex_device_object, "name", "PCR/dongle");

  while(true){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
  time_end();
}

// --------------------------------------------------------------------

