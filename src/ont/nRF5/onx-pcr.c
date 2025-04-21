
#include <onex-kernel/log.h>
#include <onex-kernel/serial.h>
#include <onex-kernel/time.h>
#include <onex-kernel/random.h>
#include <onn.h>

int main(int argc, char *argv[]) {

  properties* config = properties_new(32);
  properties_set(config, "channels", list_new_from("radio serial",2));
#define TEST_PREFIX
#ifdef  TEST_PREFIX
  properties_set(config, "test-uid-prefix", value_new("pcr"));
#endif

  time_init();
  log_init(config);
  random_init();

  onex_init(config);

  log_write("\n------Starting PCR-----\n");

  while(true){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
  time_end();
}

// --------------------------------------------------------------------

