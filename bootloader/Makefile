CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

CFLAGS  = -Os -std=gnu17 -Wall -Wextra
CFLAGS += -mcpu=cortex-m7 -mthumb -mabi=aapcs
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums

DEFINES = -DSTM32F730xx -DHSE_VALUE=24000000U -DUSE_USB_HS

INCLUDE = -IInc -IInc/HAL -IInc/CMSIS

LDFLAGS = -TSTM32F730I8KX_FLASH.ld -Wl,--gc-sections --specs=nosys.specs --specs=nano.specs

STARTUP = \
	Startup/startup_stm32f730i8kx.s

SOURCES = $(wildcard Src/*.c) \
		  $(wildcard Src/HAL/*.c)

TARGET = bootloader

all:
	$(CC) $(CFLAGS) \
		$(INCLUDE) \
		$(LDFLAGS) \
		$(STARTUP) $(SOURCES) \
		$(DEFINES) \
		-o $(TARGET).elf
	$(OBJCOPY) -Obinary $(TARGET).elf $(TARGET).bin
	$(SIZE) $(TARGET).elf

clean:
	rm -f *.o *.elf *.bin *.hex
