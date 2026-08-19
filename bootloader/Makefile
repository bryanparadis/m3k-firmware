.DEFAULT_GOAL := all

CMSIS = ../third_party/CMSIS_5/CMSIS/Core/Include
CMSIS_F7 = ../third_party/cmsis-device-f7

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

CFLAGS  = -Os -std=gnu17 -Wall -Wextra
CFLAGS += -mcpu=cortex-m7 -mthumb -mabi=aapcs
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums

DEFINES = -DHSE_VALUE=24000000U -DUSE_USB_HS

INCLUDE = -I$(CMSIS) -I$(CMSIS_F7)/Include -IInc -IInc/HAL

LDFLAGS = -TSTM32F730I8KX_FLASH.ld -Wl,--gc-sections,--no-warn-rwx-segments --specs=nano.specs # -Wl,-Map=boot.map
LDFLAGS += -Wl,--print-memory-usage

SOURCES = $(wildcard Src/*.c) \
          $(wildcard Src/HAL/*.c)

STARTUP = Startup/startup_stm32f730i8kx.s

m2k: TARGET := m2k
m2k: DEFINES += -DBOARD_M2K -DSTM32F730xx
m2k: m2k.output
m3k: TARGET := m3k
m3k: DEFINES += -DBOARD_M3K -DSTM32F723xx
m3k: m3k.output
all: m2k m3k

%.output:
	$(CC) $(CFLAGS) \
		$(INCLUDE) \
		$(LDFLAGS) \
		$(STARTUP) $(SOURCES) \
		$(DEFINES) \
		-o $(TARGET)_bootloader.elf
	$(OBJCOPY) -Obinary $(TARGET)_bootloader.elf $(TARGET)_bootloader.bin
	$(SIZE) $(TARGET)_bootloader.elf

clean:
	rm -f *.o *.elf *.bin *.hex
