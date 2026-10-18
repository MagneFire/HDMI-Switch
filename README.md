# IR-controlled HDMI switch

An ATtiny13A inside a 3-to-1 HDMI switch (cygq1080p) flips between the KPN TV box and the
Google TV Streamer when the "source" key of the KPN C3 remote (TV section programmed to LG
code 633) is pressed. It decodes the NEC frame, reads the switch's three channel LEDs and
pulls the switch's own button low until the wanted LED is lit. Prototyped on a Nano clone.

- `PLAN.md` — phases, done-criteria, wiring, test matrix, parts.
- `docs/tv-codes.md` — which remote codes to try, in order; the KPN code-list PDFs.
- `recorder/README.md` — Phase 1 recording with IRremote's ReceiveDump; the table to fill in.
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
