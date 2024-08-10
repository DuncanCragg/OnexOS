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
 -I./include/vulkan \
 -I/usr/include \
 -I/usr/include/freetype2 \
 -I./src \
 -I./src/ont/g2d \
 -I../OnexKernel/include \
 -I../OnexLang/include \

#-------------------------------------------------------------------------------

LIB_DIRS = -L/usr/lib -L../OnexLang -L../OnexKernel -Wl,-rpath,./libraries -L./libraries

LIBS_ONX_XCB = \
 -lonex-lang-x86 \
 -lonex-kernel-xcb \
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
onx-xcb: LDX=gcc
onx-xcb: ${SOURCES_ONX:.c=.o} ${HEADERS_ONX} ${SHADERS:.spv=.o}
	@echo ================
	@echo $@ '<=' ${SOURCES_ONX:.c=.o} ${SHADERS:.spv=.o}
	@echo -----
	$(LDX) -o $@ ${SOURCES_ONX:.c=.o} ${SHADERS:.spv=.o} $(LIB_DIRS) $(LIBS_ONX_XCB)

onx-drm: CONFIGFLAGS=-DVK_USE_PLATFORM_DISPLAY_KHR
onx-drm: INC_DIR=${INC_DIRS}
onx-drm: LDX=gcc
onx-drm: ${SOURCES_ONX:.c=.o} ${HEADERS_ONX} ${SHADERS:.spv=.o}
	@echo ================
	@echo $@ '<=' ${SOURCES_ONX:.c=.o} ${SHADERS:.spv=.o}
	@echo -----
	$(LDX) -o $@ ${SOURCES_ONX:.c=.o} ${SHADERS:.spv=.o} $(LIB_DIRS) $(LIBS_ONX_DRM)

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
	rm -rf ${TARGETS} src/ont/unix/*.{inc,spv,vert.c,frag.c} onx/
	rm -f core bin/core
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

SHELL=/usr/bin/bash

#-------------------------------------------------------------------------------
