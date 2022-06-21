#-------------------------------------------------------------------------------
# nRF5 Makefile

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/make clean \&\& make /'

#-------------------------------------------------------------------------------
# set a link to the nordic SDK, something like:
# ./sdk -> /home/<username>/nordic-platform/nRF5_SDK_16.0.0_98a08e2/

GCC_ARM_TOOLCHAIN = /home/duncan/gcc-arm/bin/
GCC_ARM_PREFIX = arm-none-eabi

PRIVATE_PEM = ../OnexKernel/doc/local/private.pem

#-------------------------------------------------------------------------------

EXE_DEFINES =
EXE_DEFINES += -DLOG_TO_SERIAL
EXE_DEFINES += -DHAS_SERIAL

#-------------------------------------------------------------------------------

COMMON_DEFINES = \
-DAPP_TIMER_V2 \
-DAPP_TIMER_V2_RTC1_ENABLED \
-DBOARD_PCA10059 \
-DCONFIG_GPIO_AS_PINRESET \
-DFLOAT_ABI_HARD \
-DNRF52840_XXAA \
-DNRF5 \
-DNRF_SD_BLE_API_VERSION=7 \
-DS140 \
-DSOFTDEVICE_PRESENT \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \


COMPILER_DEFINES = \
$(COMMON_DEFINES) \
$(EXE_DEFINES) \

#-------------------------------------------------------------------------------

INCLUDES = \
-I./include \
-I./src/ \
$(OL_INCLUDES) \
$(OK_INCLUDES) \
$(SDK_INCLUDES) \

#-------------------------------------------------------------------------------

EXE_SOURCES = \
./src/ont/onx-iot.c \

#-------------------------------------------------------------------------------

OL_INCLUDES = \
-I../OnexLang/include \


OK_INCLUDES = \
-I../OnexKernel/include \
-I../OnexKernel/src/platforms/nRF5 \


SDK_INCLUDES = \
-I./sdk/components/boards \
-I./sdk/components/libraries/balloc \
-I./sdk/components/libraries/experimental_section_vars \
-I./sdk/components/libraries/log \
-I./sdk/components/libraries/log/src \
-I./sdk/components/libraries/memobj \
-I./sdk/components/libraries/strerror \
-I./sdk/components/libraries/util \
-I./sdk/components/softdevice/s140/headers \
-I./sdk/components/softdevice/s140/headers/nrf52 \
-I./sdk/components/toolchain/cmsis/include \
-I./sdk/integration/nrfx/ \
-I./sdk/modules/nrfx/ \
-I./sdk/modules/nrfx/hal/ \
-I./sdk/modules/nrfx/mdk \

#-------------------------------------------------------------------------------
# Targets

onx-iot: $(EXE_SOURCES:.c=.o)
	mkdir -p okolo
	ar x ../OnexKernel/libonex-kernel-nrf.a --output okolo
	ar x ../OnexLang/libonex-lang-nrf.a     --output okolo
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES) -Wl,-Map=./onx-iot.map -o ./onx-iot.out $^ okolo/*
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size ./onx-iot.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onx-iot.out ./onx-iot.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onx-iot.out ./onx-iot.hex

flash0: onx-iot
	nrfutil pkg generate --hw-version 52 --sd-req 0xCA --application-version 1 --application ./onx-iot.hex --key-file $(PRIVATE_PEM) dfu.zip
	nrfutil dfu usb-serial -pkg dfu.zip -p /dev/ttyACM0 -b 115200

#-------------------------------------------------------------------------------

COMPILER_FLAGS = -std=c99 -MP -MD -O3 -g3 -mcpu=cortex-m4 -mthumb -mabi=aapcs -Wall -Werror -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums

LINKER_FLAGS = -O3 -g3 -mthumb -mabi=aapcs -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections --specs=nano.specs

LD_FILES = -L./sdk/modules/nrfx/mdk -T../OnexKernel/src/platforms/nRF5/onex.ld

.c.o:
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(COMPILER_FLAGS) $(COMPILER_DEFINES) $(INCLUDES) -o $@ -c $<

clean:
	find src -name '*.o' -o -name '*.d' | xargs rm -f
	find . -name onex.ondb | xargs rm -f
	rm -rf onx-iot.??? dfu.zip core okolo
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

#-------------------------------------------------------------------------------
