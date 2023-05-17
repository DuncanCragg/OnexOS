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


COMMON_DEFINES_SD = \
-DNRF_SD_BLE_API_VERSION=7 \
-DSOFTDEVICE_PRESENT \


COMPILER_DEFINES_PINETIME = \
$(COMMON_DEFINES) \
$(COMMON_DEFINES_SD) \
-DBOARD_PINETIME \
-DNRF52832_XXAA \
-DS132 \
-DNRF52 \
-DNRF52_PAN_74 \
#-DDEBUG \
#-DDEBUG_NRF \
#-DLOG_TO_RTT \
#-DLOG_TO_GFX \
#-DSPI_BLOCKING \


EPOCH_ADJUST=0

COMPILER_DEFINES_MAGIC3 = \
$(COMMON_DEFINES) \
-DBOARD_MAGIC3 \
-DNRF52840_XXAA \
-DLOG_TO_GFX \
-DEPOCH_ADJUST=$(EPOCH_ADJUST) \
#-DDEBUG \
#-DDEBUG_NRF \
#-DLOG_TO_RTT \
#-DSPI_BLOCKING \


COMPILER_DEFINES_DONGLE = \
$(COMMON_DEFINES) \
$(COMMON_DEFINES_SD) \
-DBOARD_PCA10059 \
-DNRF52840_XXAA \
-DS140 \
-DLOG_TO_SERIAL \
-DHAS_SERIAL \

#-------------------------------------------------------------------------------

INCLUDES_PINETIME = \
-I./include \
-I./src/ \
-I./external \
-I./external/fonts \
-I./external/lvgl \
-I./external/lvgl/src/lv_font \
$(OL_INCLUDES) \
$(OK_INCLUDES_PINETIME) \
$(SDK_INCLUDES_PINETIME) \


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


INCLUDES_DONGLE = \
-I./include \
-I./src/ \
$(OL_INCLUDES) \
$(OK_INCLUDES_DONGLE) \
$(SDK_INCLUDES_DONGLE) \

#-------------------------------------------------------------------------------

WEAR_SOURCES = \
./src/ont/nRF5/onx-wear.c \


SW_SOURCES = \
./src/ont/g2d/g2d.c \
./src/ont/keyboard.c \
./src/ont/user-2d.c \
./src/ont/nRF5/g2d-lcd.c \
./src/ont/nRF5/evaluators.c \
./src/ont/nRF5/onx-sw.c \


IOT_SOURCES = \
./src/ont/nRF5/onx-iot.c \


EXTERNAL_SOURCES = \
./external/fonts/fonts_noto_sans_numeric_60.c \
./external/fonts/fonts_noto_sans_numeric_80.c \

#-------------------------------------------------------------------------------

OL_INCLUDES = \
-I../OnexLang/include \


OK_INCLUDES_PINETIME = \
-I../OnexKernel/include \
-I../OnexKernel/src/onl/nRF5/pinetime \


OK_INCLUDES_MAGIC3 = \
-I../OnexKernel/include \
-I../OnexKernel/src/onl/nRF5/magic3 \


OK_INCLUDES_DONGLE = \
-I../OnexKernel/include \
-I../OnexKernel/src/onl/nRF5/dongle \


SDK_INCLUDES_PINETIME = \
-I./sdk/components/softdevice/s132/headers \
-I./sdk/components/softdevice/s132/headers/nrf52 \
-I./sdk/external/thedotfactory_fonts \
-I./sdk/components/libraries/gfx \
$(SDK_INCLUDES) \


SDK_INCLUDES_MAGIC3 = \
-I./sdk/components/drivers_nrf/nrf_soc_nosd/ \
-I./sdk/components/softdevice/mbr/headers/ \
-I./sdk/external/thedotfactory_fonts \
-I./sdk/components/libraries/gfx \
$(SDK_INCLUDES) \


SDK_INCLUDES_DONGLE = \
-I./sdk/components/softdevice/s140/headers \
-I./sdk/components/softdevice/s140/headers/nrf52 \
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

onx-wear-pinetime: INCLUDES=$(INCLUDES_PINETIME)
onx-wear-pinetime: COMPILER_DEFINES=$(COMPILER_DEFINES_PINETIME)
onx-wear-pinetime: $(EXTERNAL_SOURCES:.c=.o) $(WEAR_SOURCES:.c=.o)
	rm -rf okolo
	mkdir okolo
	ar x ../OnexKernel/libonex-kernel-pinetime.a --output okolo
	ar x   ../OnexLang/libonex-lang-nrf.a        --output okolo
	cp -a `find ~/Sources/lvgl-wear -name *.o` okolo
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_PINETIME) -Wl,-Map=./onx-wear.map -o ./onx-wear.out $^ okolo/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size --format=sysv -x ./onx-wear.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onx-wear.out ./onx-wear.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onx-wear.out ./onx-wear.hex

onx-wear-magic3: INCLUDES=$(INCLUDES_MAGIC3)
onx-wear-magic3: COMPILER_DEFINES=$(COMPILER_DEFINES_MAGIC3)
onx-wear-magic3: $(EXTERNAL_SOURCES:.c=.o) $(WEAR_SOURCES:.c=.o)
	rm -rf okolo
	mkdir okolo
	ar x ../OnexKernel/libonex-kernel-magic3.a --output okolo
	ar x   ../OnexLang/libonex-lang-nrf.a      --output okolo
	cp -a `find ~/Sources/lvgl-wear -name *.o` okolo
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_MAGIC3) -Wl,-Map=./onx-wear.map -o ./onx-wear.out $^ okolo/* -lm
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size --format=sysv -x ./onx-wear.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onx-wear.out ./onx-wear.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onx-wear.out ./onx-wear.hex

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

onx-iot: INCLUDES=$(INCLUDES_DONGLE)
onx-iot: COMPILER_DEFINES=$(COMPILER_DEFINES_DONGLE)
onx-iot: $(IOT_SOURCES:.c=.o)
	rm -rf okolo
	mkdir okolo
	ar x ../OnexKernel/libonex-kernel-dongle.a --output okolo
	ar x   ../OnexLang/libonex-lang-nrf.a      --output okolo
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(LINKER_FLAGS) $(LD_FILES_DONGLE) -Wl,-Map=./onx-iot.map -o ./onx-iot.out $^ okolo/*
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-size --format=sysv -x ./onx-iot.out
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O binary ./onx-iot.out ./onx-iot.bin
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-objcopy -O ihex   ./onx-iot.out ./onx-iot.hex

APPLICATION_VERSION = --application-version 1
BOOTLOADER_VERSION = --bootloader-version $(shell cat ../OnexKernel/bootloader-number.txt)
SETTINGS_VERSIONS = $(APPLICATION_VERSION) $(BOOTLOADER_VERSION) --bl-settings-version 2

settings.hex: onx-wear-pinetime
	nrfutil settings generate --family NRF52 --application onx-wear.hex $(SETTINGS_VERSIONS) settings.hex

pinetime-erase:
	openocd -f ../OnexKernel/doc/openocd-stlink.cfg -c init -c "reset halt" -c "nrf5 mass_erase" -c "reset run" -c exit

pinetime-flash-bootloader: settings.hex
	mergehex --merge ../OnexKernel/onex-kernel-bootloader.hex settings.hex --output ok-bl+settings.hex
	openocd -f ../OnexKernel/doc/openocd-stlink.cfg -c init -c "reset halt" -c "program ok-bl+settings.hex" -c "reset run" -c exit

pinetime-flash-sd:
	openocd -f ../OnexKernel/doc/openocd-stlink.cfg -c init -c "reset halt" -c "program ./sdk/components/softdevice/s132/hex/s132_nrf52_7.0.1_softdevice.hex" -c "reset run" -c exit

#-------------------------------:

pinetime-wear-flash: settings.hex
	mergehex --merge onx-wear.hex settings.hex --output onx-wear+settings.hex
	openocd -f ../OnexKernel/doc/openocd-stlink.cfg -c init -c "reset halt" -c "program onx-wear+settings.hex" -c "reset run" -c exit

magic3-wear-flash: onx-wear-magic3
	openocd -f ../OnexKernel/doc/openocd-stlink.cfg -c init -c "reset halt" -c "program onx-wear.hex" -c "reset run" -c exit

magic3-sw-flash: onx-sw-magic3
	openocd -f ../OnexKernel/doc/openocd-stlink.cfg -c init -c "reset halt" -c "program onx-sw.hex" -c "reset run" -c exit

dongle-flash: onx-iot
	nrfutil pkg generate --hw-version 52 --sd-req 0xCA --application-version 1 --application ./onx-iot.hex --key-file $(PRIVATE_PEM) dfu.zip
	nrfutil dfu usb-serial -pkg dfu.zip -p /dev/ttyACM0 -b 115200

#-------------------------------:

pinetime-reset:
	openocd -f ../OnexKernel/doc/openocd-stlink.cfg -c init -c "reset halt" -c "reset run" -c exit

pinetime-ota: onx-wear.hex
	nrfutil pkg generate --hw-version 52 --application onx-wear.hex $(APPLICATION_VERSION) --sd-req 0xCB --key-file $(PRIVATE_PEM) onx-wear.zip

#-------------------------------------------------------------------------------

LINKER_FLAGS = -O3 -g3 -mthumb -mabi=aapcs -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections --specs=nano.specs
LINKER_FLAGS += -Xlinker --defsym -Xlinker __BUILD_TIMESTAMP=$$(date +'%s')
LINKER_FLAGS += -Xlinker --defsym -Xlinker __BUILD_TIMEZONE_OFFSET=$$(date +'%z' | awk '{ print ($$0<0?-1:1)*((substr($$0,2,2)*3600)+(substr($$0,4,2)*60)) }')
LINKER_FLAGS += -Xlinker --defsym -Xlinker __BUILD_TIME=$$(date +'%y%m%d%H%M')
LINKER_FLAGS += -Xlinker --defsym -Xlinker __BOOTLOADER_NUMBER=$$(cat ../OnexKernel/bootloader-number.txt)

LD_FILES_PINETIME = -L./sdk/modules/nrfx/mdk -T../OnexKernel/src/onl/nRF5/pinetime/onex.ld
LD_FILES_MAGIC3   = -L./sdk/modules/nrfx/mdk -T../OnexKernel/src/onl/nRF5/magic3/onex.ld
LD_FILES_DONGLE   = -L./sdk/modules/nrfx/mdk -T../OnexKernel/src/onl/nRF5/dongle/onex.ld

COMPILER_FLAGS = -std=c99 -O3 -g3 -mcpu=cortex-m4 -mthumb -mabi=aapcs -Wall -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums

.c.o:
	$(GCC_ARM_TOOLCHAIN)$(GCC_ARM_PREFIX)-gcc $(COMPILER_FLAGS) $(COMPILER_DEFINES) $(INCLUDES) -o $@ -c $<

clean:
	find src external -name '*.o' -o -name '*.d' | xargs rm -f
	find . -name onex.ondb | xargs rm -f
	rm -rf *.hex onx-sw.??? onx-wear.??? onx-iot.??? dfu.zip core okolo
	rm -f ,*
	@echo "------------------------------"
	@echo "files not cleaned:"
	@git ls-files --others --exclude-from=.git/info/exclude | xargs -r ls -Fla

#-------------------------------------------------------------------------------
