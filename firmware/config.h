/* Project configuration. Fill in from the Phase 0/1 results (recorder/README.md, PLAN.md). */
#ifndef CONFIG_H
#define CONFIG_H

/* Trigger key on the KPN C3 remote, TV section programmed to LG code 633 (2026-10-18).
 * Starting values are the common LG TV NEC codes: address 0x04, "Input" 0x0B
 * (power 0x08, vol+ 0x02, vol- 0x03, mute 0x09). UNVERIFIED until ReceiveDump shows them.
 * Address rule: 8-bit value when the remote sends addr + ~addr (standard NEC),
 * 16-bit value otherwise (extended NEC, Samsung), i.e. as IRremote prints it. */
#define TOGGLE_ADDR   0x04
#define TOGGLE_CMD    0x0B

/* Optional: any frame from the KPN box's own address switches to the box. Leave undefined
 * until the box address is recorded. */
/* #define BOX_ADDR   0x0000 */

/* Only two sources are ever connected, and the switch never rests on an input without one,
 * so two LED senses suffice: the KPN input's LED on sense 0, the Google input's LED on
 * sense 1 (PB0/PB1 on the ATtiny, D4/D5 on the Nano). These are the read_input() results. */
#define INPUT_KPN     1      /* sense 0 lit */
#define INPUT_GOOGLE  2      /* sense 1 lit */

/* 1 if the sensed LED node goes low when the LED is on. Measured 2026-10-18: the LEDs' right
 * leg (anode) is driven to 4.2 V when lit and 0 V when not, so sense that leg: active high. */
#define LED_ACTIVE_LOW 0

/* Button press timing and retry policy. */
#define PRESS_MS      100
#define SETTLE_MS     300
#define RESAMPLE_MS   50     /* LED re-sample interval while none is lit ... */
#define RESAMPLE_MAX  20     /* ... up to RESAMPLE_MS * RESAMPLE_MAX = 1 s */
#define DEBOUNCE_MS   500
#define MAX_PRESSES   4

#endif
