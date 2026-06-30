.DEFAULT_GOAL := all

CMSIS = ../third_party/CMSIS_5/CMSIS/Core/Include
CMSIS_F7 = ../third_party/cmsis-device-f7

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

CFLAGS  = -Os -std=gnu17 -Wall -Wextra
CFLAGS += -mcpu=cortex-m7 -mthumb -mabi=aapcs
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums

INCLUDE = -I$(CMSIS) -I$(CMSIS_F7)/Include -IInc -IInc/HAL_USB

LDFLAGS = -TSTM32F730I8KX_FLASH.ld -Wl,--gc-sections,--no-warn-rwx-segments --specs=nano.specs
LDFLAGS += -Wl,--print-memory-usage

STARTUP = \
	$(CMSIS_F7)/Source/Templates/system_stm32f7xx.c

SOURCES = \
	Src/main.c \
	Src/config.c \
	Src/delay.c \
	Src/usb.c \
	Src/HAL_USB/stm32f7xx_ll_usb.c \
	Src/HAL_USB/usbd_conf.c \
	Src/HAL_USB/usbd_ctlreq.c \
	Src/HAL_USB/usbd_desc.c \
	Src/HAL_USB/usbd_hid.c \
	Src/HAL_USB/usbd_ioreq.c \
    Src/anim.c

m2k: TARGET := m2k
m2k: DEFINES += -DBOARD_M2K -DSTM32F730xx
m2k: STARTUP +=	$(CMSIS_F7)/Source/Templates/gcc/startup_stm32f730xx.s 
m2k: m2k.output
m3k: TARGET := m3k
m3k: DEFINES += -DBOARD_M3K -DSTM32F723xx
m3k: STARTUP +=	$(CMSIS_F7)/Source/Templates/gcc/startup_stm32f723xx.s 
m3k: m3k.output
all: m2k m3k

%.output:
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
