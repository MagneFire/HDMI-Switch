/* Project configuration. Fill in from the Phase 0/1 results (recorder/README.md, PLAN.md). */
#ifndef CONFIG_H
#define CONFIG_H

/* --- IR protocol ---------------------------------------------------------------------- */

#define IR_NEC   1   /* 38 kHz, 9 ms lead, 32 bits; LG, Hisense, Samsung with PROTO_SAMSUNG */
#define IR_SIRC  2   /* Sony, 40 kHz, 2.4 ms header, pulse-width bits, 3 frames per press */

/* The KPN remote's TV section must keep driving the Sony KDL-40ZX1 (volume, mute), so the
 * trigger is a Sony code. The TSOP38238 receives 40 kHz at reduced range; a TSOP38240 is
 * the exact-carrier part. */
#ifndef IR_PROTOCOL                      /* make PROTO=nec|sirc overrides */
#define IR_PROTOCOL  IR_SIRC
#endif
#define SIRC_BITS    12          /* 12 for Sony TV codes (7-bit command, 5-bit address) */

/* --- trigger key ---------------------------------------------------------------------- */

/* Recorded 2026-10-18 with the C3 on Sony code 534: 12-bit SIRC, address 1, mute 20,
 * vol+ 18, vol- 19, power 21, input 37 (the standard Sony TV set). Three frames per press,
 * 45 ms apart.
 * NEC addresses: 8-bit when the remote sends addr + ~addr, 16-bit otherwise (as IRremote
 * prints them). */
#define TOGGLE_ADDR  1
#define TOGGLE_CMD   20

/* Define to require two presses of the trigger key within this many ms. A double tap of
 * Mute mutes and unmutes the TV, net nothing. Leave undefined for a single press. */
#ifndef SINGLE_TAP                       /* make SINGLE_TAP=1 overrides */
#define DOUBLE_TAP_MS  600
#endif

/* Frames closer together than this belong to one press (Sony repeats every 45 ms, NEC2
 * every 108 ms); further apart is a new press. */
#define PRESS_GAP_MS   130

/* Optional: any frame from the KPN box's own address switches to the box. Only works if
 * the box uses the same protocol. Leave undefined until recorded. */
/* #define BOX_ADDR   0x0000 */

/* --- switch side ---------------------------------------------------------------------- */

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
#define LOCKOUT_MS    500    /* ignore new presses this long after a switch action */
#define MAX_PRESSES   4

#endif
