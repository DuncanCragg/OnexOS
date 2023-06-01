#-------------------------------------------------------------------------------
# Ubuntu + Ubuntu Touch Makefile

MAKEFLAGS += --no-builtin-rules

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' Makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/Make clean \&\& Make /'

#-------------------------------------------------------------------------------

TARGETS = onx-hwc \
          onx-pi4 \
          onx-xcb \

#-------------------------------------------------------------------------------

SOURCES_ONX_HWC = \
  ./src/ont/user-2d.c \
  ./src/ont/user-3d.c \
  ./src/ont/g2d/g2d.c \
  ./src/ont/keyboard.c \
  ./src/ont/unix/g2d-vulkan.c \
  ./src/ont/unix/onx.c \
  ./src/ont/unix/onx-vk.c \
  ./src/ont/unix/outline.c \
  ./src/ont/unix/geometry.c \
  ./src/ont/unix/vulkan/vk.c \
  ./src/onl/onl.c \
  ./src/onl/mobile/vulkan-hwc.c \

SOURCES_ONX_HWC_CPP = \
  ./src/onl/mobile/hwc.cpp \

SOURCES_ONX_XCB = \
  ./src/ont/user-2d.c \
  ./src/ont/user-3d.c \
  ./src/ont/g2d/g2d.c \
  ./src/ont/keyboard.c \
  ./src/ont/unix/g2d-vulkan.c \
  ./src/ont/unix/onx.c \
  ./src/ont/unix/onx-vk.c \
  ./src/ont/unix/outline.c \
  ./src/ont/unix/geometry.c \
  ./src/ont/unix/vulkan/vk.c \
  ./src/onl/onl.c \
  ./src/onl/desktop/vulkan-xcb.c \

#-------------------------------------------------------------------------------

HEADERS_ONX_HWC = \
  ./src/ont/unix/outline.h \
  ./src/ont/unix/geometry.h \
  ./src/ont/unix/onx-vk.h \
  ./src/onl/mobile/hwc.h \

HEADERS_ONX_XCB = \
  ./src/ont/unix/outline.h \
  ./src/ont/unix/geometry.h \
  ./src/ont/unix/onx-vk.h \

#-------------------------------------------------------------------------------

SHADERS = \
  src/ont/unix/onx.vert.spv \
  src/ont/unix/onx.frag.spv \

#-------------------------------------------------------------------------------

INC_DIR_HWC = \
 -I/opt/libhybris/include/ \
 -I/opt/libhybris/include/hybris/hwcomposerwindow \
 -I/opt/libhybris/include/hybris/eglplatformcommon \
 -I/opt/libhybris/include/hybris/platformcommon \
 -I./include \
 -I./include/libhybris \
 -I./include/vulkan \
 -I./include/android-headers \
 -I./include/freetype2 \
 -I./include/libevdev-1.0 \
 -I./src \
 -I./src/ont/g2d \
 -I../OnexLang/include \
 -I../OnexKernel/include \

INC_DIR_PI4 = \
 -I./include/vulkan \
 -I/usr/aarch64-linux-gnu/include \
 -I/usr/include \
 -I/usr/include/freetype2 \
 -I./src \
 -I./src/ont/g2d \
 -I../OnexLang/include \
 -I../OnexKernel/include \

INC_DIR_XCB = \
 -I./include/vulkan \
 -I/usr/include \
 -I/usr/include/freetype2 \
 -I./src \
 -I./src/ont/g2d \
 -I../OnexLang/include \
 -I../OnexKernel/include \

#-------------------------------------------------------------------------------

LIB_DIR_HWC = -L/opt/libhybris/lib -L./lib    -L../OnexLang -L../OnexKernel
LIB_DIR_PI4 =                      -L./lib    -L../OnexLang -L../OnexKernel
LIB_DIR_XCB =                      -L/usr/lib -L../OnexLang -L../OnexKernel

LIBS_ONX_HWC = \
 -l:libvulkan.so.1.2.183 \
 -l:libEGL.so.1.0.0 \
 -l:libhwc2.so.1 \
 -l:libhybris-hwcomposerwindow.so.1 \
 -lhybris-common \
 -lhybris-platformcommon \
 -l:libgralloc.so.1 \
 -l:libhardware.so.2 \
 -l:libsync.so.2 \
 -l:libfreetype.so.6 \
 -l:libpng12.so.0 \
 -l:libz.so.1 \
 -l:libudev.so.1 \
 -l:libinput.so.10 \
 -l:libmtdev.so.1 \
 -l:libevdev.so.2 \
 -l:libwacom.so.2 \
 -l:libgudev-1.0.so.0 \
 -l:libgobject-2.0.so.0 \
 -l:libglib-2.0.so.0 \
 -l:libffi.so.6 \
 -l:libpcre.so.3 \
 -l:libpthread-2.23.so \
 -lonex-lang-arm \
 -lonex-kernel-arm \


LIBS_ONX_PI4 = \
 -lonex-lang-arm \
 -lonex-kernel-arm \
 -lvulkan \
 -lxcb \
 -l:libfreetype.so.6.17.4 \
 -l:libbrotlidec.so.1 \
 -lbrotlicommon \
 -lXau \
 -lXdmcp \
 -l:libpng16.so.16 \
 -l:libz.so.1.2.11 \
 -lbsd \
 -lmd \
 -l:libm.so.6 \
 -l:libpthread-2.31.so \
 -l:libdl-2.31.so \
 -l:libc-2.31.so \
 -l:ld-2.31.so \


LIBS_ONX_XCB = \
 -lonex-lang-x86 \
 -lonex-kernel-x86 \
 -lvulkan \
 -lxcb \
 -lfreetype \
 -lm \


#-------------------------------------------------------------------------------

src/onl/mobile/hwc.o: ${HEADERS_ONX_HWC}

onx-hwc: CONFIGFLAGS=-DVK_USE_PLATFORM_ANDROID_KHR
onx-hwc: INC_DIR=${INC_DIR_HWC}
onx-hwc: CC=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-gcc
onx-hwc: CXX=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-g++
onx-hwc: LDX=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-g++
onx-hwc: ${SOURCES_ONX_HWC:.c=.o} ${SOURCES_ONX_HWC_CPP:.cpp=.o} ${HEADERS_ONX_HWC} ${SHADERS:.spv=.o}
	@echo ================
	@echo $@ '<=' ${SOURCES_ONX_HWC:.c=.o} ${SOURCES_ONX_HWC_CPP:.cpp=.o} ${SHADERS:.spv=.o}
	@echo -----
	$(LDX) -o $@ ${SOURCES_ONX_HWC:.c=.o} ${SOURCES_ONX_HWC_CPP:.cpp=.o} ${SHADERS:.spv=.o} $(LIB_DIR_HWC) $(LIBS_ONX_HWC)
	mkdir -p onx
	cp $@ onx

onx-pi4: CONFIGFLAGS=-DVK_USE_PLATFORM_XCB_KHR
onx-pi4: INC_DIR=${INC_DIR_PI4}
onx-pi4: CC=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-gcc
onx-pi4: LDX=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-gcc
onx-pi4: ${SOURCES_ONX_XCB:.c=.o} ${HEADERS_ONX_XCB} ${SHADERS:.spv=.o}
	@echo ================
	@echo $@ '<=' ${SOURCES_ONX_XCB:.c=.o} ${SHADERS:.spv=.o}
	@echo -----
	$(LDX) -o $@ ${SOURCES_ONX_XCB:.c=.o} ${SHADERS:.spv=.o} $(LIB_DIR_PI4) $(LIBS_ONX_PI4)
	mkdir -p onx
	cp $@ onx

onx-xcb: CONFIGFLAGS=-DVK_USE_PLATFORM_XCB_KHR
onx-xcb: INC_DIR=${INC_DIR_XCB}
onx-xcb: LDX=gcc
onx-xcb: ${SOURCES_ONX_XCB:.c=.o} ${HEADERS_ONX_XCB} ${SHADERS:.spv=.o}
	@echo ================
	@echo $@ '<=' ${SOURCES_ONX_XCB:.c=.o} ${SHADERS:.spv=.o}
	@echo -----
	$(LDX) -o $@ ${SOURCES_ONX_XCB:.c=.o} ${SHADERS:.spv=.o} $(LIB_DIR_XCB) $(LIBS_ONX_XCB)

shaderc: ${SHADERS:.spv=.c}
	@echo ================
	@echo $@ '<=' ${SHADERS:.spv=.c}

#-------------------------------------------------------------------------------

CCFLAGS  = -std=gnu17 -g -O2 -pthread -Wall -Werror -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -fno-strict-aliasing -fno-builtin-memcmp -Wimplicit-fallthrough=0 -fvisibility=hidden -Wno-unused-function -Wno-incompatible-pointer-types -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result -Wno-switch
CPPFLAGS = -std=gnu++17 -g -O2 -pthread -Wall -Werror -Wextra -Wno-unused-parameter

%.o: %.c
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	$(CC) $(CCFLAGS) $(CONFIGFLAGS) $(INC_DIR) -o $@ -c $<

%.o: %.cpp
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	$(CC) $(CPPFLAGS) $(CONFIGFLAGS) $(INC_DIR) -o $@ -c $<

%.vert.spv: %.vert
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	glslc $< -o $@

%.frag.spv: %.frag
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	glslc $< -o $@

%.vert.c: %.vert.spv
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	xxd -i $< > $@

%.frag.c: %.frag.spv
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	xxd -i $< > $@

clean:
	find . -name '*.o' | xargs rm -f
	find . -name onex.ondb | xargs rm -f
	rm -rf android/*/build android/*/.cxx/ android/.gradle/*/*
	rm -rf ${TARGETS} src/ont/unix/*.{inc,spv,vert.c,frag.c} onx/
	rm -f ,* */,* core bin/core
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

copy-android: shaderc
	(cd android; ./gradlew assembleDebug)
	adb `adb devices | grep -w device | head -1 | sed 's:\(.*\)device:-s \1:'` shell pm uninstall network.object.onexos || echo installed app not found
	adb `adb devices | grep -w device | head -1 | sed 's:\(.*\)device:-s \1:'` install android/onexos/build/outputs/apk/debug/onexos-debug.apk
	adb `adb devices | grep -w device | head -1 | sed 's:\(.*\)device:-s \1:'` shell rm -f sdcard/Onex/onex.ondb

copy-raspad: onx-pi4
	rsync -ruav --stats --progress --delete onx/ raspad:onx

copy-op5: onx-hwc
	rsync -ruav --stats --progress --delete onx/ phablet@op5:onx

copy-op5t: onx-hwc
	rsync -ruav --stats --progress --delete onx/ phablet@op5t:onx

copy-op6: onx-hwc
	rsync -ruav --stats --progress --delete onx/ phablet@op6:onx

SHELL=/usr/bin/bash

#-------------------------------------------------------------------------------
