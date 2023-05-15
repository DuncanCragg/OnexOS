
#include <pthread.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include <onn.h>
#include <onr.h>

extern bool evaluate_user(object* o, void* d);
extern object* user;
extern char* userUID;

#if defined(__ANDROID__)
extern void sprint_external_storage_directory(char* buf, int buflen, const char* format);
#endif

static object* config;
static object* oclock;

static char* clockUID=0;

static void every_second(){ onex_run_evaluators(clockUID, 0); }

static bool evaluate_default(object* o, void* d) {

  log_write("evaluate_default data=%p\n", d); object_log(o);
  return true;
}

void init_onex() {

  onex_set_evaluators((char*)"default", evaluate_default, 0);
  onex_set_evaluators((char*)"device",  evaluate_device_logic, 0);
  onex_set_evaluators((char*)"clock",   evaluate_clock, 0);
  onex_set_evaluators((char*)"user",    evaluate_user, 0);

#if defined(__ANDROID__)
  char dbpath[128];
  sprint_external_storage_directory(dbpath, 128, "%s/Onex/onex.ondb");
  log_write("Writing to DB at %s\n", dbpath);
  onex_init(dbpath);
#else
  onex_init((char*)"./onex.ondb");
#endif

  config=onex_get_from_cache((char*)"uid-0");

  if(!config){

    user=object_new(0, (char*)"user", (char*)"user", 8);
    userUID=object_property(user, (char*)"UID");

    oclock=object_new(0, (char*)"clock", (char*)"clock event", 12);
    object_set_persist(oclock, "none");
    object_property_set(oclock, (char*)"title", (char*)"OnexOS Clock");
    clockUID=object_property(oclock, (char*)"UID");

    object_set_evaluator(onex_device_object, (char*)"device");
    char* deviceUID=object_property(onex_device_object, (char*)"UID");

    object_property_add(onex_device_object, (char*)"user", userUID);
    object_property_add(onex_device_object, (char*)"io", clockUID);

    object_property_set(user, (char*)"viewing", deviceUID);

    config=object_new((char*)"uid-0", 0, (char*)"config", 10);
    object_property_set(config, (char*)"user",      userUID);
    object_property_set(config, (char*)"clock",     clockUID);
  }
  else{
    userUID=     object_property(config, (char*)"user");
    clockUID=    object_property(config, (char*)"clock");

    user     =onex_get_from_cache(userUID);
    oclock   =onex_get_from_cache(clockUID);
  }

  time_ticker(every_second, 1000);

  onex_run_evaluators(userUID, 0);
}

void loop_onex(){
  while(true){
    if(!onex_loop()){
      time_delay_ms(50); // XXX 50?
    }
  }
}

static pthread_t loop_onex_thread_id;

static void* loop_onex_thread(void* d) {
  while(true){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
  return 0;
}

void onx_init(){
#if !defined(__ANDROID__) // ugh!
  init_onex();
  pthread_create(&loop_onex_thread_id, 0, loop_onex_thread, 0);
#endif
}


