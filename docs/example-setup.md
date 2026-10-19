# Example setup: KPN C3 remote, Sony TV, KPN box + Google TV Streamer

The setup the firmware was developed on. Use it as a worked example of what to record and what
to put in `firmware/config.h`; the firmware itself knows nothing about these brands.

| Role | Here |
| --- | --- |
| Switch | 3-to-1 HDMI switch, cygq1080p |
| Input A (sense 0, PB0 / D4) | KPN TV box |
| Input B (sense 1, PB1 / D5) | Google TV Streamer |
| TV | Sony KDL-40ZX1 (1080p, HDMI 1.3) |
| Remote | KPN C3 universal remote, TV section on Sony code 534 |
| Trigger | double tap of Mute (`DOUBLE_TAP_MS 600`) |

Resulting `config.h`: `IR_PROTOCOL IR_SIRC`, `SIRC_BITS 12`, `TOGGLE_ADDR 1`, `TOGGLE_CMD 20`,
`LED_ACTIVE_LOW 0` (the LEDs' anode is driven to 4.2 V when lit).

HDMI-CEC stays **off** on the Google TV Streamer: with it on, the TV went to standby whenever
the switch moved away from it.

## Programming the C3's TV section

Code lists: `Afstandsbediening_C2_-_codes.pdf` and `Afstandsbediening_D3_-_codes.pdf` (identical
TV lists; the C3 uses the same table).

1. Hold **STOP** until the LED flashes red twice.
2. Enter the 3-digit code.
3. Test with the recorder (`recorder/README.md`), not with the TV.

## Sony codes (what is in use)

The remote must keep driving the TV's volume and mute, so the TV section takes a Sony code.
Sony is SIRC: 40 kHz, 2.4 ms header, pulse-width bits, 12 bits for TVs (address 1; mute 20,
vol+ 18, vol− 19, power 21, input 37), sent three times per press.

**Result (2026-10-18): code 534 is the standard Sony TV set and drives the KDL-40ZX1's volume
and mute.** Recorded with the probe build (`DEBUG=1`), all 12-bit SIRC, address 1:

| Key | Command | Repeat frame? | Action |
| --- | --- | --- | --- |
| Mute | 20 (0x14) | full frames, 45 ms apart, 3 per press | double tap = toggle A ⇄ B |
| Source | 37 (0x25) | same | none (TV acts on it) |
| TV power | 21 (0x15) | same | none (TV acts on it) |
| Vol+ | 18 (0x12) | same | ignore |
| Vol− | 19 (0x13) | same | ignore |
| Box OK |  |  | → A, optional (`INPUT_A_ADDR`), not recorded |
| Box channel up |  |  | → A, optional, not recorded |

Measured timing: header 2.39 ms mark / 0.6 ms space, bits 0.6 ms (0) and 1.2 ms (1) marks
with 0.6 ms spaces, frame gap about 8.5 ms after a 1-heavy frame.

Every key is a real Sony TV command, so there is no ignored key to use as a single-press
trigger. The trigger is a double tap of Mute; a lone Mute still mutes the TV.

Other Sony codes in the PDF, if 534 ever misbehaves: 540, 539, 529, 295, 541, 536, 167, 535,
294, 530, 441, 169, 533, 424, 419, 259, 185, 168, 542, 144, 852, 537, 531, 297, 237, 231.

## NEC candidate order (original plan, superseded)

The first plan was a foreign-brand code the TV ignores, so a single press could trigger. Chosen
by protocol: NEC (38 kHz, fixed 16-bit address, no toggle bit) first, Samsung32 second. Builds
with `make PROTO=nec`. Dropped because the TV then lost volume and mute from this remote.

| Order | Brand | Codes, in the PDF's order | Expected protocol |
| --- | --- | --- | --- |
| 1 | LG | 633, 630, 638, 502, 486, 602, 641, 629, 463, 481, 642, 637, 483, 833, 678, 424 | NEC (older LG sets may be RC5-like: skip those codes) |
| 2 | Hisense | 731, 734 | NEC |
| 3 | Samsung | 307, 314, 302, 308, 424, 485, 148, 306, 311, 313, 280, 505, 811, 303, 428, 502, 199, 348, 446, 315, 310, 523, 301, 249, 398, 309, 316, 522, 442, 271, 149, 827, 810, 276, 154, 153 | Samsung32 (4.5 ms header), build with `PROTO_SAMSUNG=1` |

Skip: Sony (SIRC, 40 kHz, it is the TV in use), Philips/Loewe (RC5/RC6, toggle bit),
Panasonic (Kaseikyo), Thomson (RCA, 56 kHz), Grundig/Sharp/Nokia (mixed, unknown).

Codes such as 424, 441, 446, 502 appear under several brands: they are generic library entries
and may map to any protocol. Prefer codes that are unique to the brand column.

Keep a code when all five TV keys (power, vol+, vol−, mute, source) decode as NEC (or Samsung)
with the same address, the five commands differ, and the TV shows no reaction to any of them,
both on and in standby. Otherwise move to the next code in the list.
