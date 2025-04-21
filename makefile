#-------------------------------------------------------------------------------
# nRF5 Makefile

targets:
	@grep '^[a-zA-Z0-9\.#-]\+:' makefile | grep -v '^\.' | grep -v targets | sed 's/:.*//' | uniq | sed 's/\.elf/.hex/' | sed 's/^/make clean \&\& make -j /'

#-------------------------------------------------------------------------------
# set a link to the nordic SDK, something like:
# ./sdk -> /home/<username>/nordic-platform/nRF5_SDK_16.0.0_98a08e2/

GCC_ARM_TOOLCHAIN = /home/duncan/gcc-arm/bin/
GCC_ARM_PREFIX = arm-none-eabi

PRIVATE_PEM = ../OnexKernel/doc/local/private.pem

TESTS = '1'
BUTTON ='0'
LIGHT = '0'

EXE_SOURCES =
EXE_DEFINES =

ifeq ($(TESTS), '1')
 EXE_SOURCES += $(TESTS_SOURCES)
endif

ifeq ($(BUTTON), '1')
 EXE_SOURCES += $(BUTTON_SOURCES)
endif

ifeq ($(LIGHT), '1')
 EXE_SOURCES += $(LIGHT_SOURCES)
endif

#-------------------------------------------------------------------------------

COMMON_DEFINES = \
-DAPP_TIMER_V2 \
-DAPP_TIMER_V2_RTC1_ENABLED \
-DCONFIG_GPIO_AS_PINRESET \
-DFLOAT_ABI_HARD \
-DNRF5 \
-D__HEAP_SIZE=8192 \
-D__STACK_SIZE=8192 \
-D_GNU_SOURCE \

EPOCH_ADJUST=0

COMPILER_DEFINES_MAGIC3 = \
$(COMMON_DEFINES) \
-DBOARD_MAGIC3 \
-DNRF52840_XXAA \
-DEPOCH_ADJUST=$(EPOCH_ADJUST) \
#-DDEBUG \
#-DDEBUG_NRF \
#-DSPI_BLOCKING \


COMPILER_DEFINES_ITSYBITSY = \
$(COMMON_DEFINES) \
-DBOARD_ITSYBITSY \
-DNRF52840_XXAA \


COMPILER_DEFINES_FEATHER_SENSE = \
$(COMMON_DEFINES) \
-DBOARD_FEATHER_SENSE \
-DNRF52840_XXAA \


COMPILER_DEFINES_DONGLE = \
$(COMMON_DEFINES) \
-DBOARD_PCA10059 \
-DNRF52840_XXAA \


#-------------------------------------------------------------------------------

INCLUDES_MAGIC3 = \
-I./include \
-I./src/ \
-I./src/ont/g2d \
-I./src/ont/nRF5 \
-I./external \
-I./external/fonts \
-I./external/lvgl \
-I./external/lvgl/src/lv_font \
$(OL_INCLUDES) \
$(OK_INCLUDES_MAGIC3) \
$(SDK_INCLUDES_MAGIC3) \


INCLUDES_ITSYBITSY = \
-I./include \
-I./src/ \
$(OL_INCLUDES) \
$(OK_INCLUDES_ITSYBITSY) \
$(SDK_INCLUDES_ITSYBITSY) \


INCLUDES_FEATHER_SENSE = \
-I./include \
-I./src/ \
$(OL_INCLUDES) \
$(OK_INCLUDES_FEATHER_SENSE) \
$(SDK_INCLUDES_FEATHER_SENSE) \


INCLUDES_DONGLE = \
-I./include \
-I./src/ \
$(OL_INCLUDES) \
$(OK_INCLUDES_DONGLE) \
$(SDK_INCLUDES_DONGLE) \

#-------------------------------------------------------------------------------

SW_SOURCES = \
./src/ont/g2d/g2d.c \
./src/ont/keyboard.c \
./src/ont/user-2d.c \
./src/ont/nRF5/g2d-lcd.c \
./src/ont/nRF5/evaluators.c \
./src/ont/nRF5/onx-sw.c \
./src/ont/behaviours.c \


IOT_SOURCES = \
./src/ont/behaviours.c \
./src/ont/nRF5/onx-iot.c \


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


EXTERNAL_SOURCES = \
./external/fonts/fonts_noto_sans_numeric_60.c \
./external/fonts/fonts_noto_sans_numeric_80.c \


#-------------------------------------------------------------------------------

OL_INCLUDES = \
-I../OnexLang/include \


OK_INCLUDES_MAGIC3 = \
-I../OnexKernel/include \
-I../OnexKernel/src/onl/nRF5/magic3 \


OK_INCLUDES_ITSYBITSY = \
-I../OnexKernel/include \
-I../OnexKernel/src/onl/nRF5/itsybitsy \


OK_INCLUDES_FEATHER_SENSE = \
-I../OnexKernel/include \
-I../OnexKernel/src/onl/nRF5/feather-sense \


OK_INCLUDES_DONGLE = \
-I../OnexKernel/include \
-I../OnexKernel/src/onl/nRF5/dongle \


SDK_INCLUDES_MAGIC3 = \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
-I./sdk/external/thedotfactory_fonts \
-I./sdk/components/libraries/gfx \
$(SDK_INCLUDES) \


SDK_INCLUDES_ITSYBITSY = \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
$(SDK_INCLUDES) \


SDK_INCLUDES_FEATHER_SENSE = \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
$(SDK_INCLUDES) \


SDK_INCLUDES_DONGLE = \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
$(SDK_INCLUDES) \


SDK_INCLUDES = \
-I../OnexKernel/mod-sdk/components/boards \
-I./sdk/components/libraries/balloc \
-I./sdk/components/libraries/experimental_section_vars \
-I./sdk/components/libraries/log \
-I./sdk/components/libraries/log/src \
-I./sdk/components/libraries/memobj \
-I./sdk/components/libraries/strerror \
-I./sdk/components/libraries/util \
-I./sdk/components/toolchain/cmsis/include \
-I./sdk/integration/nrfx/ \
-I./sdk/modules/nrfx/ \
-I./sdk/modules/nrfx/hal/ \
-I./sdk/modules/nrfx/mdk \

#-------------------------------------------------------------------------------
# Targets

onx-test: INCLUDES=$(INCLUDES_DONGLE)
onx-test: COMPILER_DEFINES=$(COMPILER_DEFINES_DONGLE)
onx-test: $(EXE_SOURCES:.c=.o)
	rm -rf okolo
	mkdir okolo
	ar x ../OnexKernel/libonex-kernel-dongle.a --output okolo
	ar x   ../OnexLang/libonex-lang-nrf.a      --output okolo
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_DONGLE) -Wl,-Map=./onx-test.map -o ./onx-test.out $^ okolo/*
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size --format=sysv -x ./onx-test.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onx-test.out ./onx-test.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onx-test.out ./onx-test.hex

onx-sw-magic3: INCLUDES=$(INCLUDES_MAGIC3)
onx-sw-magic3: COMPILER_DEFINES=$(COMPILER_DEFINES_MAGIC3)
onx-sw-magic3: $(SW_SOURCES:.c=.o)
	rm -rf okolo
	mkdir okolo
	ar x ../OnexKernel/libonex-kernel-magic3.a --output okolo
	ar x   ../OnexLang/libonex-lang-nrf.a      --output okolo
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_MAGIC3) -Wl,-Map=./onx-sw.map -o ./onx-sw.out $^ okolo/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size --format=sysv -x ./onx-sw.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onx-sw.out ./onx-sw.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onx-sw.out ./onx-sw.hex

onx-iot-its: INCLUDES=$(INCLUDES_ITSYBITSY)
onx-iot-its: COMPILER_DEFINES=$(COMPILER_DEFINES_ITSYBITSY)
onx-iot-its: $(IOT_SOURCES:.c=.o)
	rm -rf okolo
	mkdir okolo
	ar x ../OnexKernel/libonex-kernel-itsybitsy.a --output okolo
	ar x   ../OnexLang/libonex-lang-nrf.a      --output okolo
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_ITSYBITSY) -Wl,-Map=./onx-iot-its.map -o ./onx-iot-its.out $^ okolo/*
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size --format=sysv -x ./onx-iot-its.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onx-iot-its.out ./onx-iot-its.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onx-iot-its.out ./onx-iot-its.hex

onx-iot-fth: INCLUDES=$(INCLUDES_FEATHER_SENSE)
onx-iot-fth: COMPILER_DEFINES=$(COMPILER_DEFINES_FEATHER_SENSE)
onx-iot-fth: $(IOT_SOURCES:.c=.o)
	rm -rf okolo
	mkdir okolo
	ar x ../OnexKernel/libonex-kernel-feather-sense.a --output okolo
	ar x   ../OnexLang/libonex-lang-nrf.a             --output okolo
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_FEATHER_SENSE) -Wl,-Map=./onx-iot-fth.map -o ./onx-iot-fth.out $^ okolo/*
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size --format=sysv -x ./onx-iot-fth.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onx-iot-fth.out ./onx-iot-fth.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onx-iot-fth.out ./onx-iot-fth.hex

onx-iot-nor: INCLUDES=$(INCLUDES_DONGLE)
onx-iot-nor: COMPILER_DEFINES=$(COMPILER_DEFINES_DONGLE)
onx-iot-nor: $(IOT_SOURCES:.c=.o)
	rm -rf okolo
	mkdir okolo
	ar x ../OnexKernel/libonex-kernel-dongle.a --output okolo
	ar x   ../OnexLang/libonex-lang-nrf.a      --output okolo
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_DONGLE) -Wl,-Map=./onx-iot-nor.map -o ./onx-iot-nor.out $^ okolo/*
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size --format=sysv -x ./onx-iot-nor.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onx-iot-nor.out ./onx-iot-nor.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onx-iot-nor.out ./onx-iot-nor.hex

#-------------------------------:

dongle-test-button-light-flash: onx-test
	nrfutil pkg generate --hw-version 52 --sd-req 0x00 --application-version 1 --application ./onx-test.hex --key-file $(PRIVATE_PEM) dfu.zip
	nrfutil dfu usb-serial -pkg dfu.zip -p /dev/`ls -l /dev/nordic_dongle_flash | sed 's/.*-> //'` -b 115200

magic3-sw-flash: onx-sw-magic3
	openocd -f ../OnexKernel/doc/openocd-stlink.cfg -c init -c "reset halt" -c "program onx-sw.hex" -c "reset run" -c exit

itsybitsy-iot-flash: onx-iot-its
	uf2conv.py onx-iot-its.hex --family 0xada52840 --output onx-iot-its.uf2

feather-sense-iot-flash: onx-iot-fth
	uf2conv.py onx-iot-fth.hex --family 0xada52840 --output onx-iot-fth.uf2

dongle-iot-flash: onx-iot-nor
	nrfutil pkg generate --hw-version 52 --sd-req 0x00 --application-version 1 --application ./onx-iot-nor.hex --key-file $(PRIVATE_PEM) dfu.zip
	nrfutil dfu usb-serial -pkg dfu.zip -p /dev/`ls -l /dev/nordic_dongle_flash | sed 's/.*-> //'` -b 115200

#-------------------------------------------------------------------------------

LINKER_FLAGS = -O3 -g3 -mthumb -mabi=aapcs -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections --specs=nano.specs
LINKER_FLAGS += -Xlinker --defsym -Xlinker __BUILD_TIMESTAMP=$$(date +'%s')
LINKER_FLAGS += -Xlinker --defsym -Xlinker __BUILD_TIMEZONE_OFFSET=$$(date +'%z' | awk '{ print ($$0<0?-1:1)*((substr($$0,2,2)*3600)+(substr($$0,4,2)*60)) }')
LINKER_FLAGS += -Xlinker --defsym -Xlinker __BUILD_TIME=$$(date +'%y%m%d%H%M')

LD_FILES_MAGIC3        = -L./sdk/modules/nrfx/mdk -T../OnexKernel/src/onl/nRF5/magic3/onex.ld
LD_FILES_ITSYBITSY     = -L./sdk/modules/nrfx/mdk -T../OnexKernel/src/onl/nRF5/itsybitsy/onex.ld
LD_FILES_FEATHER_SENSE = -L./sdk/modules/nrfx/mdk -T../OnexKernel/src/onl/nRF5/feather-sense/onex.ld
LD_FILES_DONGLE        = -L./sdk/modules/nrfx/mdk -T../OnexKernel/src/onl/nRF5/dongle/onex.ld

COMPILER_FLAGS = -std=gnu17 -O3 -g3 -mcpu=cortex-m4 -mthumb -mabi=aapcs -Wall -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums

.c.o:
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(COMPILER_FLAGS) $(COMPILER_DEFINES) $(INCLUDES) -o $@ -c $<

clean:
	find src external tests -name '*.o' -o -name '*.d' | xargs rm -f
	find . -name onex.ondb | xargs rm -f
	rm -rf *.hex onx-sw.??? onx-iot*.??? onx-test.* dfu.zip core okolo
	rm -f ,*
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

#-------------------------------------------------------------------------------
