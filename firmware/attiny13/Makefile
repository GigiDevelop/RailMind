MCU = attiny13
F_CPU = 9600000UL
TARGET = marker_ir

CC = avr-gcc
OBJCOPY = avr-objcopy
CFLAGS = -std=gnu99 -Wall -Wextra -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU)

all: $(TARGET).hex

$(TARGET).elf: main.c
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

clean:
	rm -f $(TARGET).elf $(TARGET).hex

