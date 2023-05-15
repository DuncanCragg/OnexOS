
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>

#include <onex-kernel/log.h>

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

JNIEXPORT void JNICALL Java_network_object_onexos_OnexBG_loopOnex(JNIEnv* env, jclass clazz) {
  loop_onex();
}

static JavaVM* javaVM;

static bool maybe_attach_current_thread_and_fetch_env(JNIEnv** env) {

  jint res = (*javaVM)->GetEnv(javaVM, (void**)env, JNI_VERSION_1_6);
  if(res==JNI_OK) return false;

  res=(*javaVM)->AttachCurrentThread(javaVM, env, 0);
  if(res==JNI_OK) return true;

  log_write("Failed to AttachCurrentThread, ErrorCode: %d", res);
  *env=0;
  return false;
}

static jclass  onexBGClass;

// or see https://stackoverflow.com/questions/13263340/findclass-from-any-thread-in-android-jni
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {

  javaVM=vm;

  JNIEnv* env;
  bool attached=maybe_attach_current_thread_and_fetch_env(&env);
  if(!env) return 0;

  onexBGClass = (jclass)(*env)->NewGlobalRef(env, (*env)->FindClass(env, "network/object/onexos/OnexBG"));

  if(attached) (*javaVM)->DetachCurrentThread(javaVM);

  return JNI_VERSION_1_6;
}

void sprint_external_storage_directory(char* buf, int buflen, const char* format) {

  JNIEnv* env;
  bool attached=maybe_attach_current_thread_and_fetch_env(&env);
  if(!env) return;

  jclass osEnvClass = (*env)->FindClass(env, "android/os/Environment");
  jmethodID getExternalStorageDirectoryMethod = (*env)->GetStaticMethodID(env, osEnvClass, "getExternalStorageDirectory", "()Ljava/io/File;");
  jobject extStorage = (*env)->CallStaticObjectMethod(env, osEnvClass, getExternalStorageDirectoryMethod);

  jclass extStorageClass = (*env)->GetObjectClass(env, extStorage);
  jmethodID getAbsolutePathMethod = (*env)->GetMethodID(env, extStorageClass, "getAbsolutePath", "()Ljava/lang/String;");
  jstring extStoragePath = (jstring)(*env)->CallObjectMethod(env, extStorage, getAbsolutePathMethod);

  const char* extStoragePathString=(*env)->GetStringUTFChars(env, extStoragePath, 0);
  snprintf(buf, buflen, format, extStoragePathString);
  (*env)->ReleaseStringUTFChars(env, extStoragePath, extStoragePathString);

  if(attached) (*javaVM)->DetachCurrentThread(javaVM);
}


