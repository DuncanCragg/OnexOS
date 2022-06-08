# ------------------------------------

TARGETS = onx-spin-arm \
          onx-spin-x86 \

# ------------------------------------

SOURCES_ONX_SPIN_ARM = \
  ./src/ont/onx-spin.c \
  ./src/ont/outline.c \
  ./src/ont/geometry.c \
  ./src/ont/vulkan/vulkan.c \
  ./src/onl/mobile/vulkan-hwc.c \

SOURCES_ONX_SPIN_ARM_CPP = \
  ./src/onl/mobile/hwc.cpp \

SOURCES_ONX_SPIN_X86 = \
  ./src/ont/onx-spin.c \
  ./src/ont/outline.c \
  ./src/ont/geometry.c \
  ./src/ont/vulkan/vulkan.c \
  ./src/onl/desktop/vulkan-xcb.c \

# ------------------------------------

HEADERS_ONX_SPIN_ARM = \
  ./src/ont/outline.h \
  ./src/ont/geometry.h \
  ./src/ont/vulkan/vulkan.h \
  ./src/onl/mobile/hwc.h \

HEADERS_ONX_SPIN_X86 = \
  ./src/ont/outline.h \
  ./src/ont/geometry.h \
  ./src/ont/vulkan/vulkan.h \

# ------------------------------------

SHADERS = \
  src/ont/onx-spin.vert.spv \
  src/ont/onx-spin.frag.spv \

FONTS = \
  onx/fonts/Roboto-Medium.ttf

# ------------------------------------

INC_DIR_ARM = \
 -I/opt/libhybris/include/ \
 -I/opt/libhybris/include/hybris/hwcomposerwindow \
 -I/opt/libhybris/include/hybris/eglplatformcommon \
 -I/opt/libhybris/include/hybris/platformcommon \
 -I./include \
 -I./include/libhybris \
 -I./include/vulkan \
 -I./include/android-headers \
 -I./include/freetype2 \
 -I./src \

INC_DIR_X86 = \
 -I/home/duncan/Sources/vulkan-1.3.211.0/x86_64/include \
 -I/usr/include \
 -I/usr/include/freetype2 \
 -I./src \

# ------------------------------------

LIB_DIR_ARM = -L/opt/libhybris/lib -L./lib
LIB_DIR_X86 = -L/usr/lib

LIBS_ONX_SPIN_ARM = \
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

LIBS_ONX_SPIN_X86 = -lvulkan -lxcb -lm -lfreetype

# ------------------------------------

all:
	@echo "==== "${TARGETS} | sed 's: :\nmake -j6 :g'

src/onl/mobile/hwc.o: ${HEADERS_ONX_SPIN_ARM}

onx-spin-arm: CONFIGFLAGS=-DVK_USE_PLATFORM_ANDROID_KHR
onx-spin-arm: INC_DIR=${INC_DIR_ARM}
onx-spin-arm: CC=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-gcc
onx-spin-arm: CXX=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-g++
onx-spin-arm: LDX=/home/duncan/x-tools/aarch64-unknown-linux-gnu/bin/aarch64-unknown-linux-gnu-g++
onx-spin-arm: ${SOURCES_ONX_SPIN_ARM:.c=.o} ${SOURCES_ONX_SPIN_ARM_CPP:.cpp=.o} ${SHADERS} ${FONTS}
	@echo ================
	@echo $@ '<=' ${SOURCES_ONX_SPIN_ARM:.c=.o} ${SOURCES_ONX_SPIN_ARM_CPP:.cpp=.o}
	@echo -----
	$(LDX) -o $@ ${SOURCES_ONX_SPIN_ARM:.c=.o} ${SOURCES_ONX_SPIN_ARM_CPP:.cpp=.o} $(LIB_DIR_ARM) $(LIBS_ONX_SPIN_ARM)
	mkdir -p onx
	cp $@ onx

onx-spin-x86: CONFIGFLAGS=-DVK_USE_PLATFORM_XCB_KHR
onx-spin-x86: INC_DIR=${INC_DIR_X86}
onx-spin-x86: LDX=gcc
onx-spin-x86: ${SOURCES_ONX_SPIN_X86:.c=.o} ${HEADERS_ONX_SPIN_X86} ${SHADERS} ${FONTS}
	@echo ================
	@echo $@ '<=' ${SOURCES_ONX_SPIN_X86:.c=.o}
	@echo -----
	$(LDX) -o $@ ${SOURCES_ONX_SPIN_X86:.c=.o} $(LIB_DIR_X86) $(LIBS_ONX_SPIN_X86)
	mkdir -p onx
	cp $@ onx

# ------------------------------------

CCFLAGS  = -std=gnu17 -g -O2 -pthread -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -fno-strict-aliasing -fno-builtin-memcmp -Wimplicit-fallthrough=0 -fvisibility=hidden -Wno-unused-function -Wno-incompatible-pointer-types -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result -Wno-switch
CPPFLAGS = -std=gnu++17 -g -O2 -pthread

.c.o:
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	$(CC) $(CCFLAGS) $(CONFIGFLAGS) $(INC_DIR) -o $@ -c $<

.cpp.o:
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	$(CC) $(CPPFLAGS) $(CONFIGFLAGS) $(INC_DIR) -o $@ -c $<

%.vert.spv: %.vert
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	glslc $< -o $@
	mkdir -p onx/shaders
	cp $@ onx/shaders

%.frag.spv: %.frag
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	glslc $< -o $@
	mkdir -p onx/shaders
	cp $@ onx/shaders

onx/fonts/%.ttf: assets/fonts/%.ttf
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	mkdir -p onx/fonts
	cp -a $< $@

clean:
	find . -name '*.o' | xargs rm -f; rm -rf ${TARGETS} src/ont/*.{inc,spv} onx/

copy:
	rsync -ruav --stats --progress --delete onx/ phablet@dorold:onx

SHELL=/usr/bin/bash

# ------------------------------------








# ------------------------------------
