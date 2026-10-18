# KPN C3 remote: TV codes to try (Phase 0)

Source: `Afstandsbediening_C2_-_codes.pdf` and `Afstandsbediening_D3_-_codes.pdf` (identical
TV lists; the C3 uses the same table).

## Programming the TV section

1. Hold **STOP** until the LED flashes red twice.
2. Enter the 3-digit code.
3. Test with the Uno recorder (`recorder/README.md`), not with the TV.

## Sony codes (current plan)

The KPN remote must keep driving the Sony KDL-40ZX1's volume and mute, so the TV section
takes a Sony code. Sony is SIRC: 40 kHz, 2.4 ms header, pulse-width bits, 12 bits for TVs
(address 1; mute 20, vol+ 18, vol− 19, power 21, input 37), sent three times per press. The
firmware decodes it with `IR_PROTOCOL IR_SIRC`.

Try in the PDF's order and record every key with the probe: 534, 540, 539, 529, 295, 541,
536, 167, 535, 294, 530, 441, 169, 533, 424, 419, 259, 185, 168, 542, 144, 852, 537, 531,
297, 237, 231. Keep the first code where Vol+, Vol− and Mute work on the TV. Then note
whether the TV reacts to Source and to Power: a key it ignores is the cleanest trigger. If
every key works, the trigger is a double tap of Mute (`DOUBLE_TAP_MS` in config.h).

## NEC candidate order (original plan, superseded)

Chosen by protocol: NEC (38 kHz, fixed 16-bit address, no toggle bit) first, Samsung32 second.
Builds with `make PROTO=nec`.

| Order | Brand | Codes, in the PDF's order | Expected protocol |
| --- | --- | --- | --- |
| 1 | LG | 633, 630, 638, 502, 486, 602, 641, 629, 463, 481, 642, 637, 483, 833, 678, 424 | NEC (older LG sets may be RC5-like: skip those codes) |
| 2 | Hisense | 731, 734 | NEC |
| 3 | Samsung | 307, 314, 302, 308, 424, 485, 148, 306, 311, 313, 280, 505, 811, 303, 428, 502, 199, 348, 446, 315, 310, 523, 301, 249, 398, 309, 316, 522, 442, 271, 149, 827, 810, 276, 154, 153 | Samsung32 (4.5 ms header), build with `PROTO_SAMSUNG=1` |

Skip: Sony (SIRC, 40 kHz, it is the TV in use), Philips/Loewe (RC5/RC6, toggle bit),
Panasonic (Kaseikyo), Thomson (RCA, 56 kHz), Grundig/Sharp/Nokia (mixed, unknown).

Codes such as 424, 441, 446, 502 appear under several brands: they are generic library entries
and may map to any protocol. Prefer codes that are unique to the brand column.

## Keep a code when

- all five TV keys (power, vol+, vol−, mute, source) decode as NEC (or Samsung) with the same
  address,
- the five commands differ,
- the Sony Bravia shows no reaction to any of them, both on and in standby.

Otherwise move to the next code in the list.
