# Phase 1: record the IR codes

The recorder is the IRremote library's own `ReceiveDump` example. IRremote 4.7.1 is installed
in `~/Documents/Arduino/libraries/IRremote`; nothing in it needs editing.

## Wiring (Nano clone)

| TSOP38238 pin | Nano | Note |
| --- | --- | --- |
| OUT | D2 | the example's default `IR_RECEIVE_PIN` on ATmega328P |
| VCC | 5 V | through 100 Ω, with 4.7 µF from the TSOP's VCC to GND |
| GND | GND | |

## Run

1. Arduino IDE: File → Examples → IRremote → ReceiveDump. Board: Arduino Nano, processor
   ATmega328P (choose "Old Bootloader" if upload fails). Port: `/dev/cu.wchusbserial*`.
2. Serial Monitor at 115200 baud.
3. Program the C3's TV section to the candidate code (`docs/tv-codes.md`).
4. Press each of the five TV keys three times. Note protocol, address, command.
5. Hold a key for 2 s. ReceiveDump prints too slowly to show repeats, so open File → Examples →
   IRremote → ReceiveDemo for this step: it prints a compact line per frame and reports
   whether holds arrive as NEC repeat frames or as full frames (NEC2 / SamsungLG).
6. Press a few KPN box keys (OK, channel up, menu) and note the box's own protocol and address.

Keep the code when every TV key is NEC (or Samsung) with one shared address, five distinct
commands, and the Sony shows no reaction. Otherwise try the next code.

## Results

Fill in and copy the Source row into `firmware/config.h` (`TOGGLE_ADDR`, `TOGGLE_CMD`) and,
if wanted, the box address into `BOX_ADDR`.

| Key | Protocol | Address | Command | Repeat frame? | Action |
| --- | --- | --- | --- | --- | --- |
| Source |  |  |  |  | toggle KPN ⇄ Google |
| TV power |  |  |  |  | ignore |
| Vol+ |  |  |  |  | ignore |
| Vol− |  |  |  |  | ignore |
| Mute |  |  |  |  | ignore |
| Box OK |  |  |  |  | → KPN, optional |
| Box channel up |  |  |  |  | → KPN, optional |

Address format: IRremote prints an 8-bit address (`0x4`) when the second byte is the inverse
of the first (standard NEC), and a 16-bit one (`0x6B86`) otherwise (extended NEC, Samsung).
The firmware applies the same rule, so copy the value as printed.
