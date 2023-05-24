
#include <pthread.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/touch.h>

#include <onn.h>
#include <onr.h>

extern bool evaluate_user(object* o, void* d);

object* user;
object* responses;

char* useruid;
char* homeuid;
char* inventoryuid;

volatile bool         touch_down=false;
volatile touch_info_t touch_info={ 120, 140 };

bool          button_pending=false;
volatile bool button_pressed=false;

bool user_active=true;

uint32_t loop_time=0;

#if defined(__ANDROID__)
extern void sprint_external_storage_directory(char* buf, int buflen, const char* format);
#endif

static object* config;
static object* oclock;

static char* clockuid=0;

static void every_second(){ onex_run_evaluators(clockuid, 0); }

static bool evaluate_default(object* o, void* d) {

  log_write("evaluate_default data=%p\n", d); object_log(o);
  return true;
}

void init_onex() {

  onex_set_evaluators("default", evaluate_default, 0);
  onex_set_evaluators("device",  evaluate_device_logic, 0);
  onex_set_evaluators("clock",   evaluate_clock, 0);
  onex_set_evaluators("user",    evaluate_user, 0);

#if defined(__ANDROID__)
  char dbpath[128];
  sprint_external_storage_directory(dbpath, 128, "%s/Onex/onex.ondb");
  log_write("Writing to DB at %s\n", dbpath);
  onex_init(dbpath);
#else
  onex_init("./onex.ondb");
#endif

  object* home;
  object* inventory;

  config=onex_get_from_cache("uid-0");

  if(!config){

    user=object_new(0, "user", "user", 8);
    useruid=object_property(user, "UID");

    home=object_new(0, "editable",  "list editable", 4);
    homeuid=object_property(home, "UID");

    inventory=object_new(0, "editable",  "list editable", 4);
    inventoryuid=object_property(inventory, "UID");

    responses=object_new(0, "default",   "user responses", 12);

    oclock=object_new(0, "clock", "clock event", 12);
    object_set_persist(oclock, "none");
    object_property_set(oclock, "title", "OnexOS Clock");
    clockuid=object_property(oclock, "UID");

    object_set_evaluator(onex_device_object, "device");
    char* deviceuid=object_property(onex_device_object, "UID");

    object_property_add(onex_device_object, "user", useruid);
    object_property_add(onex_device_object, "io", clockuid);

    config=object_new("uid-0", 0, "config", 10);
    object_property_set(config, "user",      useruid);
    object_property_set(config, "clock",     clockuid);

    object_property_set(user, "viewing", clockuid);
  }
  else{
    useruid=     object_property(config, "user");
    clockuid=    object_property(config, "clock");

    user     =onex_get_from_cache(useruid);
    oclock   =onex_get_from_cache(clockuid);
  }

  time_ticker(every_second, 1000);

  onex_run_evaluators(useruid, 0);
}

static void* do_onex_loop(void* d) {
  while(true){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
  return 0;
}

#if defined(__ANDROID__)

// Android - OnexBG - calls init_onex() and loop_onex() itself
// onx_init() is the call-up from the event loop once "prepared"
// can be called before or after init_onex()
void onx_init(){
  onex_run_evaluators(useruid, 0);
}

void loop_onex(){
  do_onex_loop(0);
}

#else

static pthread_t loop_onex_thread_id;

void onx_init(){
  init_onex();
  pthread_create(&loop_onex_thread_id, 0, do_onex_loop, 0);
}

#endif

