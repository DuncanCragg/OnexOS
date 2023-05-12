
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>

#include <onex-kernel/log.h>
#include <android/log.h>
#define log_init()
#define log_loop() true
#undef  log_write
#define log_write(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OnexOS", __VA_ARGS__))
#define log_flush()

extern void init_onex();
extern void loop_onex();
extern void on_alarm_recv(char*);

// extern void serial_on_recv(char*, int);
void serial_on_recv(char* b, size_t len)
{
  // was in deleted onl/android
}


JNIEXPORT void JNICALL Java_network_object_onexos_OnexBG_initOnex(JNIEnv* env, jclass clazz) {
  init_onex();
}

JNIEXPORT void JNICALL Java_network_object_onexos_OnexBG_loopOnex(JNIEnv* env, jclass clazz)
{
  loop_onex();
}


bool focused = true;

void android_main(struct android_app* state) {

  while(1){

    int ident;
    int events;
    struct android_poll_source* source;
    bool destroy = false;
    while((ident = ALooper_pollAll(focused ? 0 : 200, NULL, &events, (void**)&source)) >= 0){

      if(source) {
        source->process(state, source);
      }
    }
    if(!focused){
      if(ident==ALOOPER_POLL_TIMEOUT) continue;
    }
    if(destroy){
      break;
    }
  }
}


