
#include <pthread.h>
#include <string.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onex-kernel/touch.h>

#include <onn.h>
#include <onr.h>
#include <ont.h>

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

bool display_on=true;

uint32_t loop_time=0;

static object* config;
static object* oclock;

static char* clockuid=0;

static void every_second(void*){
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

  properties* config = properties_new(32);
  properties_set(config, "dbpath", value_new("./onex.ondb"));
  properties_set(config, "channels", list_new_from_fixed("ipv6"));
  properties_set(config, "ipv6_groups", list_new_from_fixed("ff12::1234"));
  onex_init(config);

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
    char* floorpaneluid;

    floorpanel=object_new(0, "editable", "3d cuboid", 4);
    floorpaneluid=object_property(floorpanel, "UID");

    object_property_add(floorpanel, "shape", "[1.5/0.1/1.5]");

    for(int x=0; x<2; x++){
      for(int y=0; y<2; y++){
        for(int z=0; z<2; z++){

          object* p=object_new(0, "editable", "3d cuboid", 4);
          char*   puid=object_property(p, "UID");

          object_property_add(p, "shape", "[0.5/0.02/0.5]");

          char v[64]; snprintf(v, 64, "[%.2f/%.2f/%.2f]", x*1.4f-0.7f, y*0.7f+0.7f, z*1.4f-0.7f);

          object_property_add(floorpanel, "position", v);
          object_property_add(floorpanel, "contains", puid);

          for(int xx=0; xx<2; xx++){
            for(int yy=0; yy<2; yy++){
              for(int zz=0; zz<2; zz++){

                object* pp=object_new(0, "editable", "3d cuboid", 4);
                char*   ppuid=object_property(pp, "UID");

                object_property_add(pp, "shape", "[0.05/0.05/0.05]");

                char vv[64]; snprintf(vv, 64, "[%.2f/%.2f/%.2f]", xx*.14f, yy*.14f+.14f, zz*.14f);

                object_property_add(p, "position", vv);
                object_property_add(p, "contains", ppuid);
              }
            }
          }
        }
      }
    }

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

  time_ticker(every_second, 0, 1000);

  onex_run_evaluators(useruid, 0);
}

static void* do_onex_loop(void*) {
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

