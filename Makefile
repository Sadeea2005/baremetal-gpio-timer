# Cross-compile the bare-metal firmware for STM32F103 (Cortex-M3).
# Produces blink.elf and a raw blink.bin you can flash with st-flash / dfu.
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

CPU     = -mcpu=cortex-m3 -mthumb
CFLAGS  = $(CPU) -std=c11 -Wall -Wextra -O2 -g -ffreestanding -nostdlib
LDFLAGS = $(CPU) -T stm32f103.ld -nostdlib -Wl,--gc-sections

SRCS    = startup.c main.c
TARGET  = blink

.PHONY: all clean
all: $(TARGET).bin

$(TARGET).elf: $(SRCS) stm32f103.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $@
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Flash to a Blue Pill via ST-Link (needs stlink tools installed):
#   make flash
flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000

clean:
	rm -f $(TARGET).elf $(TARGET).bin
