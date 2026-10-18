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

/* Which switch input (1..3, as the LEDs are numbered) each source is on. */
#define INPUT_KPN     1
#define INPUT_GOOGLE  2

/* 1 if the sensed LED node goes low when the LED is on (Phase 0 measurement). */
#define LED_ACTIVE_LOW 0

/* Button press timing and retry policy. */
#define PRESS_MS      100
#define SETTLE_MS     300
#define RESAMPLE_MS   20
#define DEBOUNCE_MS   500
#define MAX_PRESSES   4

#endif
