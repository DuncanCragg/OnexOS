
#include <pthread.h>
#include <string.h>

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

static object* config;
static object* oclock;

static char* clockuid=0;

static void every_second(){
  onex_run_evaluators(clockuid, 0);
  onex_run_evaluators(useruid, 0);
}

static bool evaluate_default(object* o, void* d) {

  log_write("evaluate_default data=%p\n", d); object_log(o);
  return true;
}

static char note_text[] = "the fat cat sat on me the quick brown fox jumps over the lazy doggie";

void init_onex() {

  onex_set_evaluators("default", evaluate_default, 0);
  onex_set_evaluators("device",  evaluate_device_logic, 0);
  onex_set_evaluators("clock",   evaluate_clock, 0);
  onex_set_evaluators("user",    evaluate_user, 0);
  onex_set_evaluators("notes",   evaluate_edit_rule, 0);

  onex_init("./onex.ondb");

  object* home;
  object* inventory;

  object* note;
  char* noteuid;

  config=onex_get_from_cache("uid-0");

  if(!config){

    user=object_new(0, "user", "user", 8);
    useruid=object_property(user, "UID");

    home=object_new(0, "editable",  "list editable", 4);
    homeuid=object_property(home, "UID");

    inventory=object_new(0, "editable",  "list editable", 4);
    inventoryuid=object_property(inventory, "UID");

    // -----------

    object* floorpanel;
    object* backpanel;
    object* roofpanel;

    char* floorpaneluid;
    char* backpaneluid;
    char* roofpaneluid;

    floorpanel=object_new(0, "editable", "3d cuboid", 4);
    floorpaneluid=object_property(floorpanel, "UID");

    backpanel=object_new(0, "editable", "3d cuboid", 4);
    backpaneluid=object_property(backpanel, "UID");

    roofpanel=object_new(0, "editable", "3d cuboid", 4);
    roofpaneluid=object_property(roofpanel, "UID");

    object_property_set_list(floorpanel, "shape", "1.5", "0.1", "1.5", 0);
    object_property_set_list(floorpanel, "position-1", "0.0", "1.5", "-1.5", 0);
    object_property_add(     floorpanel, "contains-1", backpaneluid);

    object_property_set_list(backpanel, "shape", "1.5", "1.5", "0.1", 0);
    object_property_set_list(backpanel, "position-1", "0.0", "1.5", "0.0", 0);
    object_property_add(     backpanel, "contains-1", roofpaneluid);

    object_property_set_list(roofpanel,  "shape", "1.5", "0.1", "1.5", 0);

    // -----------

    note=object_new(0, "notes", "text editable", 4);
    noteuid=object_property(note, "UID");

    char* strtok_state = 0;
    char* word = strtok_r(note_text, " ", &strtok_state);
    while(word){
      object_property_add(note, "text", word);
      word = strtok_r(0, " ", &strtok_state);
    }

    // -----------

    responses=object_new(0, "default",   "user responses", 12);

    // -----------

    oclock=object_new(0, "clock", "clock event", 12);
    object_set_persist(oclock, "none");
    object_property_set(oclock, "title", "OnexOS Clock");
    clockuid=object_property(oclock, "UID");

    // -----------

    object_set_evaluator(onex_device_object, "device");
    char* deviceuid=object_property(onex_device_object, "UID");

    object_property_add(onex_device_object, "user", useruid);
    object_property_add(onex_device_object, "io", clockuid);

    // -----------

    config=object_new("uid-0", 0, "config", 10);
    object_property_set(config, "user",      useruid);
    object_property_set(config, "clock",     clockuid);

    // -----------

    object_property_set(user, "viewing", floorpaneluid);
//  object_property_set(user, "viewing", clockuid);
//  object_property_set(user, "viewing", noteuid);
//  object_property_set(user, "viewing", deviceuid);
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

static pthread_t loop_onex_thread_id;

void ont_vk_init(){
  init_onex();
  pthread_create(&loop_onex_thread_id, 0, do_onex_loop, 0);
}

