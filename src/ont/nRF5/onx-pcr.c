
#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onn.h>
#include <onr.h>
#include <ont.h>

int main() {

  properties* config = properties_new(32);
  properties_set(config, "channels", list_vals_new_from_fixed("radio serial"));
#define TEST_MODE
#ifdef  TEST_MODE
  properties_set(config, "flags", list_vals_new_from_fixed("debug-on-serial log-onp log-to-led"));// debug-on-serial log-onp
  properties_set(config, "test-uid-prefix", value_new("pcr"));
#endif

  time_init();

  log_init(config);
  if(debug_on_serial){
    while(!serial_ready_state()){ time_delay_ms(100); serial_loop(); }
  }

  random_init();

  log_write("\n------Starting PCR-----\n");

  onex_init(config);

  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);
  object_set_evaluator(onex_device_object, "evaluate_device");
  object_property_set(onex_device_object, "name", "PCR/dongle");

  while(true){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
}

// --------------------------------------------------------------------

