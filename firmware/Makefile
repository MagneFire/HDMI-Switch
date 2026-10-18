# IR-controlled HDMI switch firmware. Two targets, one source tree.
#
#   make                      ATmega328P (Nano/Uno prototype)
#   make MCU=attiny13a        ATtiny13A (final)
#   make size                 flash / RAM report for the chosen MCU
#   make DEBUG=1              UART trace at 115200 (ATmega328P only)
#   make PROTO_SAMSUNG=1      also accept Samsung32 frames
#   make flash-uno            avrdude through the Nano bootloader (PORT=, BAUD=57600 for old)
#   make flash-t13            avrdude through USBtinyISP
#   make fuses-t13            9.6 MHz, CKDIV8 off, BOD off, reset kept (set once)

MCU ?= atmega328p

ifeq ($(MCU),attiny13a)
F_CPU ?= 9600000
else
F_CPU ?= 16000000
endif

# Homebrew's avr-gcc@14 is keg-only; avr-binutils (objcopy, size) is a separate formula.
BREW_GCC := $(shell brew --prefix avr-gcc@14 2>/dev/null)
BREW_BIN := $(shell brew --prefix avr-binutils 2>/dev/null)
CC      := $(if $(BREW_GCC),$(BREW_GCC)/bin/,)avr-gcc
OBJCOPY := $(if $(BREW_BIN),$(BREW_BIN)/bin/,)avr-objcopy
SIZE    := $(if $(BREW_BIN),$(BREW_BIN)/bin/,)avr-size
OBJDUMP := $(if $(BREW_BIN),$(BREW_BIN)/bin/,)avr-objdump
AVRDUDE ?= avrdude

CFLAGS := -std=gnu11 -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL \
          -Wall -Wextra -Wundef \
          -ffunction-sections -fdata-sections -fno-tree-scev-cprop \
          -fshort-enums -fpack-struct -mrelax
LDFLAGS := -mmcu=$(MCU) -Wl,--gc-sections -Wl,--relax

ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG
endif
ifeq ($(PROTO_SAMSUNG),1)
CFLAGS += -DPROTO_SAMSUNG
endif

OUT  := build/$(MCU)
SRC  := main.c
OBJ  := $(SRC:%.c=$(OUT)/%.o)
ELF  := $(OUT)/hdmi_switch.elf
HEX  := $(OUT)/hdmi_switch.hex

PORT ?= $(firstword $(wildcard /dev/cu.wchusbserial* /dev/cu.usbserial* /dev/ttyUSB*))
BAUD ?= 57600   # this Nano has the old bootloader; new ones take 115200
ISP_BITCLOCK ?=

.PHONY: all size clean flash-uno flash-t13 fuses-t13 disasm

all: $(HEX) size

$(OUT):
	mkdir -p $@

$(OUT)/%.o: %.c hal.h config.h Makefile | $(OUT)
	$(CC) $(CFLAGS) -c -o $@ $<

$(ELF): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

size: $(ELF)
	$(SIZE) -C --mcu=$(MCU) $<

disasm: $(ELF)
	$(OBJDUMP) -d $< | less

flash-uno: $(HEX)
	$(AVRDUDE) -c arduino -p m328p -P $(PORT) -b $(BAUD) -U flash:w:$<:i

flash-t13: $(HEX)
	$(AVRDUDE) -c usbtiny -p t13 $(ISP_BITCLOCK) -U flash:w:$<:i

fuses-t13:
	$(AVRDUDE) -c usbtiny -p t13 $(ISP_BITCLOCK) -U lfuse:w:0x7a:m -U hfuse:w:0xff:m

clean:
	rm -rf build
