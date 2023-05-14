
project(OnexOS)

cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

set(TOP_DIR ../..)
set(ONEX_KERNEL ../../../OnexKernel)
set(ONEX_LANG ../../../OnexLang)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu17 -Wall -Werror -Wno-incompatible-pointer-types-discards-qualifiers -Wno-incompatible-pointer-types -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -DVK_USE_PLATFORM_ANDROID_KHR")

file(GLOB SOURCE_FILES
"${ONEX_KERNEL}/src/onl/unix/channel-serial.c"
"${ONEX_KERNEL}/src/onl/unix/mem.c"
"${ONEX_KERNEL}/src/onl/unix/persistence.c"
"${ONEX_KERNEL}/src/onl/unix/properties.c"
"${ONEX_KERNEL}/src/onl/unix/random.c"
"${ONEX_KERNEL}/src/onl/unix/serial.c"
"${ONEX_KERNEL}/src/onl/unix/time.c"
"${ONEX_KERNEL}/src/lib/lib.c"
"${ONEX_KERNEL}/src/lib/list.c"
"${ONEX_KERNEL}/src/lib/value.c"
"${ONEX_KERNEL}/src/onn/*.c"
"${ONEX_KERNEL}/src/onp/*.c"
"${ONEX_LANG}/src/*.c"
"${TOP_DIR}/src/onl/onl.c"
"${TOP_DIR}/src/onl/android/*.c"
"${TOP_DIR}/src/ont/unix/vulkan/*.c"
"${TOP_DIR}/src/ont/unix/*.c"
)

add_library(native-lib SHARED ${SOURCE_FILES})

add_library(native-app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

include_directories(${ANDROID_NDK}/sources/android/native_app_glue)
include_directories(${ONEX_KERNEL}/include)
include_directories(${ONEX_KERNEL}/src)
include_directories(${ONEX_KERNEL}/src/onn)
include_directories(${ONEX_KERNEL}/src/onp)
include_directories(${ONEX_LANG}/include)
include_directories(${TOP_DIR}/include/vulkan)
include_directories(${TOP_DIR}/external/freetype/include)
include_directories(${TOP_DIR}/src)

add_subdirectory(${TOP_DIR}/external/freetype ${TOP_DIR}/external/freetype)

target_link_libraries(
    native-lib
    native-app-glue
    android
    log
    z
    vulkan
    freetype
)
