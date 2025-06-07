#include <time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <mathlib.h>
#include <onex-kernel/log.h>
#include <onex-kernel/time.h>
#include <items.h>
#include <onn.h>
#include <ont.h>

static bool discover_io_peer(object* o, char* property, char* is) {

  char ispath[32]; snprintf(ispath, 32, "%s:is", property);

  if(object_property_contains(o, ispath, is)) return true;

  int ln=object_property_length(o, "device:peers:io");
  for(int i=1; i<=ln; i++){
    char* uid=object_property_get_n(o, "device:peers:io", i);
    if(!is_uid(uid)) continue;
    object_property_set(o, property, uid);
    if(object_property_contains(o, ispath, is)) return true;
  }
  if(ln) object_property_set(o, property, 0);
  return false;
}

bool evaluate_light_logic(object* o, void* d){

  bool light_on=object_property_is(o, "light", "on");

  if(light_on && object_property_is(o, "Timer", "0")){

    object_property_set(o, "Timer", "");
    object_property_set(o, "light", "off");

    return true;
  }

  if(!light_on && (
       object_property_is(o, "button:state",   "down") ||
       object_property_is(o, "touch:action",   "down") ||
       object_property_is(o, "motion:gesture", "view-screen"))){

    light_on=true;
    object_property_set(o, "light", "on");
  }

  if(light_on /* && !object_property(o, "Timer") */ )  {

    char* timeout=object_property(o, "timeout");
    if(timeout){
      object_property_set(o, "Timer", timeout);
    }
  }

  object_property(o, "button:is"); // REVISIT: "observe the button"?

  bool has_bcs_link     = object_property(o, "bcs:1:is");
  bool has_compass_link = object_property(o, "compass:1:is");
  bool has_touch_link   = object_property(o, "touch:1:is");

  if(has_bcs_link || has_compass_link || has_touch_link){

    uint8_t brightness = 0xff;
    uint8_t colour     = 0xff;
    uint8_t softness   = 0x00;

    if(has_bcs_link){
      brightness = (uint8_t)object_property_int32(o, "bcs:1:brightness");
      colour     = (uint8_t)object_property_int32(o, "bcs:1:colour");
      softness   = (uint8_t)object_property_int32(o, "bcs:1:softness");
    }
#ifdef DO_COMPASS_LINK
    if(has_compass_link){
      int32_t direction = object_property_int32(o, "compass:1:direction");
      colour = (uint8_t)((direction + 180)*256/360);
    }
#endif
#ifdef DO_TOUCH_LINK
    if(has_touch_link){
      int32_t touch_x = object_property_int32(o, "touch:1:coords:1");
      int32_t touch_y = object_property_int32(o, "touch:1:coords:2");
      bool    touch_d = object_property_is(   o, "touch:1:action", "down");
      colour   = (255-touch_x) & 0xff;
      softness = (    touch_y) & 0xff;
    }
#endif
    object_property_set_fmt(o, "colour", "%%%02x%02x%02x", brightness, colour, softness);
  }

  if(object_property(o, "touch:is") ||
     object_property(o, "motion:is")   ) return true;

  discover_io_peer(o, "button", "button");
#ifdef DO_TOUCH_LINK
  discover_io_peer(o, "touch", "touch");
#endif

  if(light_on && object_property_is(o, "button:state", "up")){

    object_property_set(o, "light", "off");
  }
  return true;
}

bool evaluate_device_logic(object* o, void* d)
{
  if(object_property_contains(o, "Alerted:is", "device")){
    char* devuid=object_property(o, "Alerted");
    // REVISIT: no notification of local to remote if "already seen" it
    if(!object_property_contains(o, "peers", devuid)){
      object_property_add(o, "peers", devuid);
    }
  }
  return true;
}

bool evaluate_clock_sync(object* o, void* d) {

  if(!discover_io_peer(o, "sync-clock", "clock")) return true;

  char* sync_ts=object_property(o, "sync-clock:ts");

  if(!sync_ts || object_property_is(o, "sync-ts", sync_ts)) return true;

  object_property_set(o, "sync-ts",  sync_ts);
  object_property_set(o, "ts",  sync_ts);
  object_property_set(o, "tz",  object_property(o, "sync-clock:tz:1"));
  object_property_add(o, "tz",  object_property(o, "sync-clock:tz:2"));

  char* e; uint64_t sync_clock_ts=strtoull(sync_ts,&e,10);
  if(!*e && sync_clock_ts) time_es_set(sync_clock_ts);

  return true;
}

#if defined(NRF5)
extern char __BUILD_TIMEZONE_OFFSET;
#endif

bool evaluate_clock(object* o, void* d)
{
  uint64_t es=time_es();

  char ess[16];
#if defined(NRF5)
  if(es>>32){
    // sort this out in 2038
    log_write("timestamp overflow\n");
    uint32_t lo=es & 0xffffffff;
    uint32_t hi=(es>>32);
    snprintf(ess, 16, "%lu:%lu", hi, lo);
  }
  else snprintf(ess, 16, "%"PRIu32, (uint32_t)es);
#else
  snprintf(ess, 16, "%"PRIu64, es);
#endif

  if(object_property_is(o, "ts", ess)) return true; // XXX remove when no-change OK

  object_property_set(o, "ts", ess);

  if(!object_property(o, "sync-clock")){
    char*   zone=0;
    int16_t offs=0;
#if !defined(NRF5)
    time_t est = (time_t)es;
    struct tm tms={0};
    localtime_r(&est, &tms);
    zone=tms.tm_zone;
    offs=tms.tm_gmtoff;
#else
    offs=(int16_t)(int32_t)&__BUILD_TIMEZONE_OFFSET;
    if(offs==3600) zone="BST"; // probably...
    else
    if(offs==0)    zone="GMT";
    else           zone="XXX"; // probably not...
#endif
    char t[32];
    snprintf(t, 32, "%s %d", zone, offs);
    object_property_set(o, "tz", t);
  }

  return true;
}

