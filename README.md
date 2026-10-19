# IR-controlled HDMI switch

An ATtiny13A inside a 3-to-1 HDMI switch (cygq1080p) toggles between two sources when a chosen
key on a programmable remote is double-tapped (or pressed once). It decodes the IR frame (NEC or
Sony SIRC), reads two of the switch's channel LEDs to see which input is active, and pulls the
switch's own button low until the wanted LED is lit. The switch keeps its button and LEDs; the
MCU only adds a hand that presses and eyes that read. Prototyped on a Nano clone.

Adapting it to your setup: program the remote's TV section to a code (one the TV ignores for a
single-press trigger, or the TV's own code with a double tap of Mute), record the keys with the
probe build (`recorder/README.md`), then fill in `firmware/config.h`: `IR_PROTOCOL`,
`TOGGLE_ADDR`, `TOGGLE_CMD`, `LED_ACTIVE_LOW`, and which LED is input A and which is input B.
`docs/example-setup.md` is a complete worked example.

- `PLAN.md` — the build log: phases, measurements, wiring, test matrix, parts.
- `docs/example-setup.md` — the author's setup: remote codes, recorded keys, resulting config.
- `recorder/README.md` — recording IR codes with the probe build or IRremote's ReceiveDump.
- `firmware/` — plain C for ATmega328P and ATtiny13A; `config.h` holds the recorded codes.

## Build

```sh
make -C firmware                    # ATmega328P, build/atmega328p/hdmi_switch.hex
make -C firmware MCU=attiny13a      # ATtiny13A
make -C firmware DEBUG=1            # UART trace at 115200 on the Nano
make -C firmware flash-uno          # Nano bootloader; BAUD=57600 for the old bootloader
make -C firmware MCU=attiny13a fuses-t13 flash-t13   # USBtinyISP
```

Toolchain: Homebrew `avr-gcc@14` (keg-only, the Makefile finds it) and `avrdude`.
