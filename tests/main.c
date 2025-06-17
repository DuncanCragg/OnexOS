
#if defined(NRF5)
#include <boards.h>
#include <onex-kernel/boot.h>
#include <onex-kernel/gpio.h>
#include <onex-kernel/serial.h>
#endif
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/random.h>
#include <tests.h>
#include <onn.h>

extern void run_light_tests();
extern void run_device_tests();
extern void run_clock_tests();

#if defined(NRF5)
#if defined(BOARD_PCA10059)
const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;
#endif

static void set_up_gpio(void)
{
#if defined(BOARD_PCA10059)
  for(uint8_t l=0; l< LEDS_NUMBER; l++){ gpio_mode(leds_list[l], OUTPUT); gpio_set(leds_list[l], 1); }
  gpio_set(leds_list[0], 0);
#endif
}
#endif

static volatile char char_recvd=0;
static volatile bool run_tests =false;

void serial_cb(bool connect, char* tty){
  if(connect) return;
#if defined(NRF5)
  char chars[1024];
  serial_read(chars, 1024);
  char_recvd=chars[0];
#else
  char_recvd='t';
#endif
  if(char_recvd=='t') run_tests=true;
}

void run_tests_maybe(properties* config) {

  if(!run_tests) return;
  run_tests=false;

  log_write("ONR tests\n");

  onex_init(config);

  run_light_tests();
  run_clock_tests();

  onex_loop(); time_delay_ms(150);
  onex_loop(); time_delay_ms(150);

#if defined(NRF5)
  int failures=onex_assert_summary();
#if defined(BOARD_PCA10059)
  if(failures) gpio_set(leds_list[1], 0);
  else         gpio_set(leds_list[2], 0);
#endif
#else
  onex_assert_summary();
#endif
}

#if defined(NRF5)
static void loop_serial(void*){ serial_loop(); }
#endif

int main() {

  properties* config = properties_new(32);
#if defined(NRF5)
#if !defined(BOARD_MAGIC3)
  properties_set(config, "flags", list_vals_new_from_fixed("debug-on-serial log-to-led"));
#else
  properties_set(config, "flags", list_vals_new_from_fixed("log-to-gfx"));
#endif
#else
  properties_set(config, "dbpath", value_new("tests.ondb"));
#endif
  properties_set(config, "test-uid-prefix", value_new("tests"));

  time_init();

  log_init(config);

#if defined(NRF5) && !defined(BOARD_MAGIC3)
  serial_init(0,0,serial_cb); // overrides log's one
  serial_ready_state(); // blocks until stable at start
  time_ticker(loop_serial, 0, 1);
#endif

  random_init();
#if defined(NRF5)
  gpio_init();
  set_up_gpio();

  while(1){
    if(char_recvd){
      log_write(">%c<----------\n", char_recvd);
      char_recvd=0;
    }
    run_tests_maybe(config);
    log_loop();
  }
#else
  serial_cb(false, "tty");
  run_tests_maybe(config);
  time_end();
#endif
}

