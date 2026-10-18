# IR-controlled HDMI switch — build plan

Oct 18, 2026 · Darrel

## Goal

A small MCU inside a 3-to-1 HDMI switch (cygq1080p) selects the KPN TV box or the Google TV Streamer when a chosen key on the KPN remote is pressed. The MCU decodes IR, reads the switch's three channel LEDs, and "presses" the switch's own button until the right LED is lit.

Build order: prove everything on the Nano clone (ATmega328P, CH340C, AMS1117-5.0 — called "the Uno" below, it's the same chip and logic level; recorder and prototype), then port to an ATtiny13A in SOIC-8 (on hand), small enough to sit on the switch PCB with no case modification. Budget: 1 KB flash, 64 B RAM, so the firmware is written lean from the start; an ATtiny85 is the fallback if it won't fit. Each phase has a done-criterion; don't move on until it passes.

The switch keeps its own button and LEDs; the MCU only adds a hand that presses and eyes that read.

## Phase 0 — Reconnaissance (multimeter only)

Done when: the C3 remote's TV keys are programmed to a non-Sony code that the Bravia ignores and that decodes cleanly, and you have the switch's button polarity, LED node voltages and supply rail written down.

- [ ] Trigger keys = the C3's TV section (TV power, vol+, vol−, mute, source), reprogrammed to a foreign brand so the Sony Bravia never reacts. The C3 uses the C2/D3 code lists; programming: hold STOP until the LED flashes red twice, enter the 3-digit code. You give up TV volume/power from the KPN remote; the Google TV remote or the Sony remote covers that.
- [ ] Choose the brand by protocol, not by name. Want: 38 kHz carrier, a fixed address, no toggle bit, so the 13A decoder stays tiny. Good: LG (NEC), Hisense (NEC), Samsung (Samsung32, 4.5 ms header). Avoid: Sony (SIRC, 40 kHz, your TV), Philips and Loewe (RC5/RC6, 36 kHz, toggle bit), Panasonic (Kaseikyo 48-bit), Thomson (RCA, 56 kHz).
- [ ] Program the first LG code, run the Uno `ReceiveDump` (Phase 1), press all five TV keys. Keep the code if: every key decodes as NEC with the same address, all five commands differ, and the Bravia shows no reaction with the TV on and in standby. Otherwise try the next LG code, then Hisense, then Samsung. Candidate order: `docs/tv-codes.md`.
- [ ] Open the switch. Photograph the board; note the IC marking, any LDO (SOT-23 or SOT-89 near the IC) and its output voltage.
- [ ] With a source plugged in, measure: the 5 V rail (from HDMI pin 18), the LDO output (expect 3.3 V), the button's idle and pressed voltage (expect idle high, pressed = GND), and each LED node (which side swings, and to what voltage).
- [ ] Press the button and watch whether the LEDs are steady or multiplexed (a scope, or a phone slow-mo video, shows flicker).
- [ ] Note the auto-switch behaviour: power-cycle one source and see whether the switch jumps to it.
- [ ] Decide the power source: tap the switch's 5 V rail (OK if the extra ~2 mA leaves margin) or feed 5 V from a TV USB port into the same rail. USB is the safer default.

Constraint: HDMI-CEC stays **off** on the Google TV Streamer. With CEC on it put the TV in standby whenever the switch moved away from it. Check the KPN box's CEC setting too, and expect the switch not to pass CEC reliably anyway; the IR path is the only control channel this design relies on.

| Measurement | Value | Notes |
| --- | --- | --- |
| HDMI 5 V rail |  | not yet measured |
| LDO output | ~3.45 V (inferred from the button pull-up) | 2026-10-18 |
| Button idle / pressed | 3.45 V / GND | one side on GND, the other pulled up: open-drain press is right |
| LED node high / low | right leg (anode, driven): 4.2 V lit / 0 V not lit; left leg (cathode): 1.7 V lit / 0 V | active-high on the anode side. Sense the right leg through 10 kΩ; `LED_ACTIVE_LOW 0`. 4.2 V is above a 3.3 V ATtiny's VCC: the 10 kΩ keeps the clamp current under 0.1 mA |
| LEDs multiplexed? |  | not yet checked |
| Auto-switch on source power-on? | yes: with one source attached it always selects that one | a press blanks all LEDs for a split second, then it re-selects the same input. Firmware re-samples the LEDs for up to 1 s after a press |

Notes from the measurements:

- The MCU can run from the ~3.45 V rail. The LED senses then see 4.2 V, above VCC: keep the
  10 kΩ series resistor (clamp current < 0.1 mA) or use a 10 k / 22 k divider.
- With only one source attached, the toggle cannot leave that input; test with both sources on.

### Board notes (from the photo)

- Inputs J1/J4/J5 top, output J3 bottom, main IC (QFN, CYGQ1080P) centre-left, SW2 centre, LEDs D5/D6/D8 in a row with R9/R10/R13 beside them.
- D27/D28 (and D1 by J1) are the ORing diodes merging the inputs' HDMI 5 V onto one rail. Feed TV-USB 5 V into that node through your own Schottky. The capacitive-touch module's 5 V wire already taps it near J3.
- U2 (SOT-23-5) is the LDO; with L2 nearby there may also be a buck for the core. Measure U2's output: if it's 1.8 V rather than 3.3 V, run the ATtiny from the 5 V rail instead and rely on the series resistors and open-drain press.
- SW2 already has the touch-switch wires on it; the ATtiny joins that node. Check the touch module's output stage: push-pull would fight the ATtiny's open-drain low. Add a 1 kΩ series resistor on the ATtiny side, or remove the module.
- Free flat area on the right (around the FM1 fiducial and C27) for a dead-bug SOIC-8 and the TSOP.
- U7/U8 by J3 (U8 populated, SOT-23 marked "-A") and the 2×7 pad row bottom-left: ignored for now; not on the path this project touches.

## Phase 1 — Record the IR codes

Done when: you have protocol, address and command for every trigger key, and you've confirmed the repeat-frame behaviour. Steps and the table: `recorder/README.md`.

1. Wire a TSOP38238 to the Uno: OUT → D2, VCC → 5 V, GND → GND, 100 Ω + 4.7 µF on VCC as the datasheet suggests.
2. Install the IRremote library (Arduino IDE → Library Manager) and run its `ReceiveDump` example at 115200 baud.
3. With the C3 programmed to the candidate code, press each of the five TV keys three times. Record protocol, address, command, and whether repeats arrive as a short NEC repeat frame or as full frames.
4. Press a few box keys (OK, channel, menu) too and record the box's own protocol and address: the MCU can treat *any* box code as "switch to KPN", which gives you a hands-free fallback.
5. If the candidate decodes as something other than NEC/Samsung, go back to Phase 0 and pick the next code.
6. Pick the toggle key (source is the natural one) and write it in the table. The other TV keys are ignored, so an accidental press does nothing.

| Key | Protocol | Address | Command | Repeat frame? | Action |
| --- | --- | --- | --- | --- | --- |
| Source |  |  |  |  | toggle KPN ⇄ Google |
| Mute, Vol+, Vol−, TV power |  |  |  |  | ignore |
| Any box key |  |  |  |  | → KPN, optional |

## Phase 2 — Uno prototype

Done when: a trigger key on the remote reliably lands the switch on the right input from any starting input, 20 times in a row, with the TV showing the right picture.

### Wiring

| Signal | Uno pin | Notes |
| --- | --- | --- |
| IR receiver OUT | D2 | pin-change or external interrupt |
| Switch button | D3 | open-drain: INPUT idle, OUTPUT LOW to press; 1 kΩ series |
| KPN input's LED sense | D4 | 10 kΩ series, on the LED's right leg (anode, 4.2 V lit / 0 V off) |
| Google input's LED sense | D5 | same |
| GND | GND | common with the switch board |

Keep the Uno's 5 V off the switch board; share only GND. The series resistors and open-drain press keep 5 V logic from stressing a 3.3 V IC.

If the LED node swings less than ~2.5 V, the Uno won't read it as HIGH: use the analog inputs (A0–A2) with a threshold instead. On the ATtiny the same pins are ADC-capable, so this ports.

### Firmware (write it to port)

- No IRremote in the final sketch. Write a ~60-line NEC decoder on a pin-change interrupt: measure mark/space, assemble 32 bits, verify `cmd == ~cmd_inv`, emit a code once per press and ignore repeat frames.
- Timing without `micros()`: on the Uno it's fine to use it, but write the decoder against a `tick()` function you can swap. On the 13A, `tick()` reads Timer0 (prescaler /64 at 9.6 MHz = 6.7 µs per tick) plus an overflow counter so the 9 ms NEC lead fits. Keep all timing in 8-/16-bit integers, no floats, no division.
- Read the KPN and Google LEDs into a 2-bit value; exactly one bit set = that input; 0 or both = unknown → keep re-reading every 50 ms for up to 1 s (the switch blanks the LEDs after a press).
- Toggle logic: on the toggle code, read the current input first. `target = (current == KPN) ? GOOGLE : KPN`. Input 3 or unknown also goes to KPN, so the first press from any odd state lands somewhere sensible.
- State machine: `IDLE` → on code set `target` → `PRESS` (hold low 100 ms, release 300 ms, let the switch settle) → re-read LEDs → match ⇒ `IDLE`, else `PRESS` again, max 4 presses, then give up.
- Debounce: ignore a new code within 500 ms of the last. This matters more with a toggle: a repeat frame or a double-decode would flip it straight back.
- Keep RAM to a handful of globals (state, target, last code, tick counters). No `String`, no arrays larger than needed. `Serial` only inside `#ifdef DEBUG` for the Uno build.
- Write the prototype in plain C against the ATmega328P registers (PCINT on PD2, Timer0), not as an Arduino sketch: then `make MCU=attiny13a` from day one shows the real flash size, and Phase 3 is a header swap. IRremote stays only in the Phase 1 throwaway recorder sketch.

### Test matrix

- [ ] From KPN, press the toggle key → lands on Google. Press again → back on KPN. Repeat 10 times.
- [ ] From input 3 (press the switch's own button) → toggle lands on KPN.
- [ ] Hold the key for 2 s → exactly one flip, not two.
- [ ] Press twice quickly (< 500 ms apart) → one flip.
- [ ] Press from 3 m and from an angle → still triggers (receiver placement).
- [ ] Power-cycle the Google TV Streamer → note whether the switch auto-jumps; if so the next toggle still reads the real state and does the right thing.
- [ ] Run for an evening of normal use; count misfires.

## Phase 3 — ATtiny13A port

Done when: the ATtiny13A on a SOIC-8 breakout passes the same test matrix as the Uno, powered from 3.3 V, with flash under 1024 B and RAM under 64 B reported by the compiler.

### Pin map

| ATtiny13A pin | Signal | Why here |
| --- | --- | --- |
| PB0 (MOSI) | KPN input's LED sense | high-impedance input, harmless on ISP |
| PB1 (MISO) | Google input's LED sense | same |
| PB2 (SCK) | free | only two sources are ever connected, so the third LED is not sensed |
| PB3 (ADC3) | IR receiver OUT | off the ISP lines (receiver output could fight the programmer) |
| PB4 (ADC2) | Button (open-drain) | off the ISP lines |
| PB5 | RESET | keep reserved; using it needs high-voltage programming |

The 13A and 85 share this pinout, so the Uno-as-ISP wiring and the breakout are the same if you ever fall back to an 85. Two LED senses are enough: the switch never rests on an input without a source (Phase 0), and "neither lit" still shows the blank after a press.

### Toolchain

1. Bare metal: `avr-gcc`, `avr-libc`, `avrdude` (Debian/Ubuntu: `gcc-avr avr-libc avrdude`; macOS: `brew tap osx-cross/avr && brew install avr-gcc@14 avrdude` — keg-only, so the Makefile points at `$(brew --prefix avr-gcc@14)/bin/avr-gcc`). No Arduino core on the 13A: its `init()` and timer ISR cost ~100–150 B of the 1 KB and you'd be fighting it for Timer0 anyway.
2. One source tree, two targets: `main.c` + `hal.h` with the pin masks, PCINT setup and `tick()`; `Makefile` with `MCU=atmega328p F_CPU=16000000` for the Uno and `MCU=attiny13a F_CPU=9600000` for the 13A. Flags: `-Os -mmcu=$(MCU) -DF_CPU=$(F_CPU) -ffunction-sections -fdata-sections -Wl,--gc-sections`.
3. Fuses on the 13A: `lfuse=0x7A hfuse=0xFF` = 9.6 MHz internal, no CKDIV8, BOD off (the chip ships at 1.2 MHz). Set once with avrdude `-U lfuse:w:0x7a:m -U hfuse:w:0xff:m`.
4. Programmer: the Arduino ISP (ATtiny44, USBtinyISP firmware, 6-pin ICSP, supplies 5 V). avrdude: `-c usbtiny -p t13`, no port needed; add `-B 32` if a slow-clocked chip fails to initialise. Fallback, if the ISP dies: the Uno running `ArduinoISP` (File → Examples), 10 µF cap between Uno RESET and GND, wires D10→RESET, D11→MOSI, D12→MISO, D13→SCK, 5 V, GND. avrdude: `-c stk500v1 -P /dev/ttyACM0 -b 19200 -p t13`. For the Nano clone itself: CH340 appears as /dev/cu.wchusbserial\* on macOS; old bootloader = -b 57600, new = -b 115200: `-c arduino -P /dev/cu.wchusbserial* -b 115200 -p m328p` (its bootloader).
5. For the SOIC-8 chip: solder it to a SOIC-8-to-DIP breakout for bring-up, and get a SOIC-8 test clip (Pomona 5250 or a cheap clone) to reflash it once it's on the switch board.
6. Check the build output every time: `avr-size -C --mcu=attiny13a` must stay under 1024 B flash / 64 B data+bss. A NEC decoder plus this state machine should land around 400–600 B.

### Programming strategy

Program on the breakout during bring-up with nothing else connected. Once the chip is on the switch board, reflash in place through the test clip: lift the switch's 3.3 V from the chip's VCC (a solder jumper or just power the board from the programmer alone), since the programmer's 5 V would backfeed the LDO. The LED senses on the ISP lines are high-impedance so they won't fight the programmer; the receiver and button stay on PB3/PB4 for that reason.

### Port checklist

- [ ] Compile the Uno sketch for the 13A with `DEBUG` off and look at the size. Trim until it fits with ≥ 150 B headroom.
- [ ] Replace `micros()` with the Timer0 `tick()`; the decoder's ±25 % tolerances should still pass at 6.7 µs resolution.
- [ ] Pin-change interrupt: the 13A has one PCINT vector for all PB pins; mask on PB3 only.
- [ ] No `millis()`: derive the 100 / 300 / 500 ms delays from the same Timer0 overflow counter.
- [ ] Drop the status-LED/error blink if flash is tight; it's the first thing to go.
- [ ] Run from 3.3 V on the breakout; confirm the LED thresholds (direct digital reads if the IC is 3.3 V).
- [ ] Re-run the Phase 2 test matrix.
- [ ] If it still won't fit after trimming, fall back to an ATtiny85 (same pinout, ATTinyCore, 8 MHz) rather than fighting for the last 50 bytes.

## Phase 4 — Integration and enclosure

Done when: the switch is closed up, sits behind the TV, and a week of normal use has no misfires.

- [ ] Mount the SOIC-8 ATtiny13A dead-bug or on a sliver of SOIC breakout glued to a free spot on the switch PCB; 100 nF right at its VCC/GND. The TSOP38238 and its RC filter fit beside it.
- [ ] Power: wire the chip's VCC to the switch's LDO output (3.3 V). If the LDO is missing or the IC runs at 5 V, run the 13A at 5 V and keep the open-drain press and series resistors.
- [ ] If using USB power: a short cable from the TV's USB port to the switch's 5 V rail. Add a Schottky diode so the TV's 5 V and the sources' 5 V don't fight.
- [ ] Solder the button line to the switch pad; solder LED senses to the swinging side of each LED. Use 30 AWG wire-wrap wire.
- [ ] IR receiver: it must see the remote. The switch housing is closed, so either place the TSOP behind the existing LED window if IR passes through it (test first), or put it on 30 cm of twisted wire out through the HDMI-side opening and stick it to the TV bezel.
- [ ] Re-run the test matrix with the lid on.
- [ ] Give it a week. Note any auto-switch surprises (source power-on, TV standby) and add a correction in firmware if flash allows.

## Parts

| Part | Qty | Notes |
| --- | --- | --- |
| Nano clone (ATmega328P) | 1 | recorder, prototype; the Pro Micro (32U4) is a spare recorder |
| ATtiny13A SOIC-8 | 2 | on hand; one spare |
| SOIC-8 to DIP breakout | 1 | bring-up |
| SOIC-8 test clip | 1 | in-circuit reflash |
| TSOP38238 (38 kHz IR receiver) | 2 | one for the Uno, one in the final build |
| 100 Ω resistor, 4.7 µF cap | 1 each | TSOP supply filter |
| 100 nF ceramic (0603 or 0805) | 2 | decoupling |
| 1 kΩ, 10 kΩ resistors | few | button series, LED senses |
| Arduino ISP (USBtiny, 6-pin ICSP) | 1 | on hand |
| Schottky diode (1N5819) | 1 | only if USB-powered |
| 30 AWG wire-wrap wire | — |  |
| ATtiny85-20PU | optional | fallback if the 13A is too tight |

## Open questions

- [ ] Does IR reach the inside of the closed housing (through the LED window or vents), or does the receiver need a wire out? Decides whether "no case modification" holds.
- [ ] Does the Google TV Streamer negotiate cleanly through the switch's EDID? If it comes up blank or flickers, set its output to 1080p/60 SDR manually in its display settings. The TV is a Sony KDL-40ZX1 (1080p, HDMI 1.3), so the 1080p switch loses nothing.
