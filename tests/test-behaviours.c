
#include <stdio.h>
#include <inttypes.h>
#include <tests.h>
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <items.h>
#include <ont.h>

// ---------------------------------------------------------------------------------

uint8_t evaluate_button_io_called=0;
bool evaluate_button_io(object* button, void* pressed) {
  evaluate_button_io_called++;
  if(evaluate_button_io_called==1) onex_assert( !pressed, "evaluate_button_io arg is false the 1st time");
  if(evaluate_button_io_called==2) onex_assert(!!pressed, "evaluate_button_io arg is true  the 2nd time");
  if(evaluate_button_io_called==3) onex_assert( !pressed, "evaluate_button_io arg is false the 3rd time");
  if(evaluate_button_io_called==4) onex_assert( !pressed, "evaluate_button_io arg is false the 4th time");
  if(evaluate_button_io_called==5) onex_assert(!!pressed, "evaluate_button_io arg is true  the 5th time");
  char* s=(pressed? "down": "up");
  object_property_set(button, "state", s);
  return true;
}

uint8_t evaluate_light_io_called=0;
bool evaluate_light_io(object* light, void* d) {
  evaluate_light_io_called++;
  if(evaluate_light_io_called==1) onex_assert(object_property_is(light, "light", "on"),  "evaluate_light_io light is on  the 1st time");
  if(evaluate_light_io_called==2) onex_assert(object_property_is(light, "light", "off"), "evaluate_light_io light is off the 2nd time");
  if(evaluate_light_io_called==3) onex_assert(object_property_is(light, "light", "on"),  "evaluate_light_io light is on  the 3rd time");
  if(evaluate_light_io_called==4) onex_assert(object_property_is(light, "light", "off"), "evaluate_light_io light is off the 4th time");
  if(evaluate_light_io_called==5) onex_assert(object_property_is(light, "light", "on"),  "evaluate_light_io light is on  the 5th time");
  return true;
}

void run_light_tests() {

  log_write("------light behaviour tests-----\n");

  onex_set_evaluators("eval_button", evaluate_button_io, 0);
  onex_set_evaluators("eval_light", evaluate_light_logic, evaluate_light_io, 0);

  object* button=object_new(0, "eval_button", "button", 4);
  object* light =object_new(0, "eval_light",  "light", 4);

  char* buttonuid=object_property(button, "UID");
  char* lightuid=object_property(light, "UID");

  object_property_set(light, "light", "on");
  object_property_set(light, "button", buttonuid);

  onex_run_evaluators(lightuid, 0); // first time, 5 more...

  onex_loop();
  bool button_pressed=false;
  onex_run_evaluators(buttonuid, (void*)button_pressed);

  onex_loop();
  onex_loop();
  button_pressed=true;
  onex_run_evaluators(buttonuid, (void*)button_pressed);

  onex_loop();
  onex_loop();
  button_pressed=false;
  onex_run_evaluators(buttonuid, (void*)button_pressed);

  onex_loop();
  onex_loop();
  button_pressed=false;
  onex_run_evaluators(buttonuid, (void*)button_pressed);

  onex_loop();
  onex_loop();
  button_pressed=true;
  onex_run_evaluators(buttonuid, (void*)button_pressed);

  onex_loop();
  onex_loop();
  onex_assert_equal_num(evaluate_button_io_called, 5,  "evaluate_button_io was called five times");
  onex_assert_equal_num(evaluate_light_io_called,  5,  "evaluate_light_io  was called five times");
}

void run_device_tests() {

  log_write("------device behaviour tests-----\n");

  onex_set_evaluators("evaluate_device", evaluate_device_logic, 0);
  object_set_evaluator(onex_device_object, "evaluate_device");

  object_property_set(onex_device_object, "incoming", "uid-incomingdevice");
  object_property(onex_device_object, "incoming:UID");
  object* incomingdevice=onex_get_from_cache("uid-incomingdevice");
  object_property_set(incomingdevice, "is", "device");
  onex_loop();

  onex_assert_equal(object_property(onex_device_object, "peers"), "uid-incomingdevice", "device evaluator adds incoming device to peers");
}

void run_clock_tests() {

  log_write("------clock behaviour tests-----\n");

  onex_set_evaluators("eval_clock",                           evaluate_clock, 0);
  onex_set_evaluators("eval_clock_sync", evaluate_clock_sync, evaluate_clock, 0);

  object* clock_synced_from=object_new(0, "eval_clock",      "clock event", 12);
  object* clock_to_sync    =object_new(0, "eval_clock_sync", "clock event", 12);

  char* clock_synced_from_uid=object_property(clock_synced_from, "UID");
  char* clock_to_sync_uid    =object_property(clock_to_sync, "UID");

  object_property_set(clock_to_sync, "sync-clock", clock_synced_from_uid);

  time_es_set(12345555);

  onex_run_evaluators(clock_synced_from_uid, 0);
  onex_run_evaluators(clock_to_sync_uid, 0);

  onex_loop();

  onex_assert_equal(object_property(   clock_synced_from, "ts"),  "12345555", "sync clock sets epoch");
  onex_assert_equal(object_property(   clock_to_sync,     "ts"),  "12345555", "clocks synced");
  onex_assert(      object_property_is(clock_to_sync,     "tz:1", "GMT") ||
                    object_property_is(clock_to_sync,     "tz:1", "BST"),     "timezone id synced" );
  onex_assert(      object_property_is(clock_to_sync,     "tz:2", "0") ||
                    object_property_is(clock_to_sync,     "tz:2", "3600"),    "timezone offset synced" );

  object_property_set(clock_synced_from, "ts", "12345678");
  object_property_set(clock_synced_from, "tz", "XYZ 1234");

  onex_loop();

  onex_assert_equal_num(time_es(), 12345678, "epoch clock set");

  onex_assert_equal(object_property(   clock_to_sync, "sync-ts"), "12345678", "clocks synced");
  onex_assert_equal(object_property(   clock_to_sync, "ts"),      "12345678", "clocks synced");
  onex_assert(      object_property_is(clock_to_sync,  "tz:1", "XYZ"),        "timezone id synced" );
  onex_assert(      object_property_is(clock_to_sync,  "tz:2", "1234"),       "timezone offset synced" );
}

