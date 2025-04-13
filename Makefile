#-------------------------------------------------------------------------------
# Ubuntu + Ubuntu Touch Makefile

MAKEFLAGS += --no-builtin-rules

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' Makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/Make clean \&\& Make -j /'

#-------------------------------------------------------------------------------

TARGETS = onx-xcb \
          onx-drm \

#-------------------------------------------------------------------------------

SOURCES_ONX = \
  ./src/ont/user-2d.c \
  ./src/ont/user-3d.c \
  ./src/ont/g2d/g2d.c \
  ./src/ont/keyboard.c \
  ./src/ont/unix/g2d-vulkan.c \
  ./src/ont/unix/onx.c \
  ./src/ont/unix/ont-vk.c \
  ./src/ont/unix/outline.c \
  ./src/ont/unix/geometry.c \



BUTTON_SOURCES = \
./src/ont/behaviours.c \
./src/ont/button-light/button.c \


LIGHT_SOURCES = \
./src/ont/behaviours.c \
./src/ont/button-light/light.c \


TESTS_SOURCES = \
./src/ont/behaviours.c \
./tests/test-behaviours.c \
./tests/main.c \


#-------------------------------------------------------------------------------

HEADERS_ONX = \
  ./src/ont/unix/outline.h \
  ./src/ont/unix/geometry.h \

#-------------------------------------------------------------------------------

SHADERS = \
  src/ont/unix/onx.vert.spv \
  src/ont/unix/onx.frag.spv \

#-------------------------------------------------------------------------------

INC_DIRS = \
 -I./include \
 -I./include/vulkan \
 -I/usr/include \
 -I/usr/include/freetype2 \
 -I./src \
 -I./src/ont/g2d \
 -I../OnexKernel/include \
 -I../OnexLang/include \

#-------------------------------------------------------------------------------

LIB_DIRS = -L/usr/lib -L../OnexLang -L../OnexKernel -Wl,-rpath,./libraries -L./libraries


LIBS_ONEX = \
 -lonex-lang-x86 \
 -lonex-kernel-xcb \


LIBS_ONX_XCB = \
 $(LIBS_ONEX) \
 -lviture_one_sdk \
 -lvulkan \
 -lxcb \
 -levdev \
 -lfreetype \
 -lm \


LIBS_ONX_DRM = \
 -lonex-lang-x86 \
 -lonex-kernel-drm \
 -lviture_one_sdk \
 -lvulkan \
 -levdev \
 -lfreetype \
 -lm \


#-------------------------------------------------------------------------------

onx-xcb: CONFIGFLAGS=-DVK_USE_PLATFORM_XCB_KHR
onx-xcb: INC_DIR=${INC_DIRS}
onx-xcb: CC=gcc
onx-xcb: LD=gcc
onx-xcb: ${SOURCES_ONX:.c=.o} ${HEADERS_ONX} ${SHADERS:.spv=.o}
	@echo ================
	@echo $@ '<=' ${SOURCES_ONX:.c=.o} ${SHADERS:.spv=.o}
	@echo -----
	$(LD) -o $@ ${SOURCES_ONX:.c=.o} ${SHADERS:.spv=.o} $(LIB_DIRS) $(LIBS_ONX_XCB)

onx-drm: CONFIGFLAGS=-DVK_USE_PLATFORM_DISPLAY_KHR
onx-drm: INC_DIR=${INC_DIRS}
onx-drm: CC=gcc
onx-drm: LD=gcc
onx-drm: ${SOURCES_ONX:.c=.o} ${HEADERS_ONX} ${SHADERS:.spv=.o}
	@echo ================
	@echo $@ '<=' ${SOURCES_ONX:.c=.o} ${SHADERS:.spv=.o}
	@echo -----
	$(LD) -o $@ ${SOURCES_ONX:.c=.o} ${SHADERS:.spv=.o} $(LIB_DIRS) $(LIBS_ONX_DRM)

shaderc: ${SHADERS:.spv=.c}
	@echo ================
	@echo $@ '<=' ${SHADERS:.spv=.c}

run.xcb: onx-xcb
	rm -f onex.ondb
	sudo prlimit --core=unlimited ./onx-xcb

run.valgrind: onx-xcb
	rm -f onex.ondb
	valgrind --leak-check=yes --undef-value-errors=no ./onx-xcb

#-------------------------------------------------------------------------------

button.x86: INC_DIR=${INC_DIRS}
button.x86: CC=gcc
button.x86: LD=gcc
button.x86: ${BUTTON_SOURCES:.c=.o}
	@echo ================
	@echo $@ '<=' ${BUTTON_SOURCES:.c=.o}
	@echo -----
	$(LD) -o $@ ${BUTTON_SOURCES:.c=.o} $(LIB_DIRS) $(LIBS_ONEX)

light.x86: INC_DIR=${INC_DIRS}
light.x86: CC=gcc
light.x86: LD=gcc
light.x86: ${LIGHT_SOURCES:.c=.o}
	@echo ================
	@echo $@ '<=' ${LIGHT_SOURCES:.c=.o}
	@echo -----
	$(LD) -o $@ ${LIGHT_SOURCES:.c=.o} $(LIB_DIRS) $(LIBS_ONEX)

tests.x86: INC_DIR=${INC_DIRS}
tests.x86: CC=gcc
tests.x86: LD=gcc
tests.x86: ${TESTS_SOURCES:.c=.o}
	@echo ================
	@echo $@ '<=' ${TESTS_SOURCES:.c=.o}
	@echo -----
	$(LD) -o $@ ${TESTS_SOURCES:.c=.o} $(LIB_DIRS) $(LIBS_ONEX)

x86.button: button.x86
	./button.x86

x86.light: light.x86
	./light.x86

x86.tests: tests.x86
	./tests.x86

#-------------------------------------------------------------------------------

DOBUG_FLAGS= -g     -O2
DEBUG_FLAGS= -ggdb3 -O0

CCFLAGS  = $(DEBUG_FLAGS) -std=gnu17 -pthread -Wall -Werror -Wextra -Wno-discarded-qualifiers -Wno-unused-parameter -Wno-missing-field-initializers -fno-strict-aliasing -fno-builtin-memcmp -Wimplicit-fallthrough=0 -fvisibility=hidden -Wno-unused-function -Wno-incompatible-pointer-types -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result -Wno-switch

# CCFLAGS = -std=gnu99 -Wno-pointer-sign -Wno-format -Wno-sign-compare -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-write-strings -Wno-old-style-declaration -Wno-strict-aliasing -fno-common -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer

%.o: %.c
	@echo ================
	@echo $@ '<=' $<
	@echo -----
	$(CC) $(CCFLAGS) $(CONFIGFLAGS) $(INC_DIR) -o $@ -c $<

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
	rm -rf ${TARGETS} button.* light.* tests.* src/ont/unix/*.{inc,spv,vert.c,frag.c} onx/
	rm -f core.*
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

SHELL=/usr/bin/bash

#-------------------------------------------------------------------------------
