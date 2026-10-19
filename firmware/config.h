/* Project configuration. Fill in from the Phase 0/1 results (recorder/README.md, PLAN.md). */
#ifndef CONFIG_H
#define CONFIG_H

/* --- IR protocol ---------------------------------------------------------------------- */

#define IR_NEC   1   /* 38 kHz, 9 ms lead, 32 bits; LG, Hisense, Samsung with PROTO_SAMSUNG */
#define IR_SIRC  2   /* Sony, 40 kHz, 2.4 ms header, pulse-width bits, 3 frames per press */

/* Pick the protocol the remote's chosen code speaks. If the remote must keep working the TV,
 * the trigger key is one of the TV's own codes (see DOUBLE_TAP_MS). Match the receiver to the
 * carrier: a TSOP38238 receives 40 kHz at reduced range; a TSOP38240 is the exact-carrier
 * part. */
#ifndef IR_PROTOCOL                      /* make PROTO=nec|sirc overrides */
#define IR_PROTOCOL  IR_SIRC
#endif
#define SIRC_BITS    12          /* 12 for SIRC TV codes (7-bit command, 5-bit address) */

/* --- trigger key ---------------------------------------------------------------------- */

/* Address and command of the trigger key as the probe build prints them (recorder/README.md).
 * Example values from docs/example-setup.md: 12-bit SIRC, address 1, command 20 (Mute), three
 * frames per press, 45 ms apart.
 * NEC addresses: 8-bit when the remote sends addr + ~addr, 16-bit otherwise (as IRremote
 * prints them). */
#define TOGGLE_ADDR  1
#define TOGGLE_CMD   20

/* Define to require two presses of the trigger key within this many ms. Lets a key the TV
 * acts on serve as the trigger: a double tap of Mute mutes and unmutes, net nothing. Leave
 * undefined for a single press of a key the TV ignores. */
#ifndef SINGLE_TAP                       /* make SINGLE_TAP=1 overrides */
#define DOUBLE_TAP_MS  600
#endif

/* Frames closer together than this belong to one press (SIRC repeats every 45 ms, NEC2
 * every 108 ms); further apart is a new press. */
#define PRESS_GAP_MS   130

/* Optional: any frame from this address selects input A, e.g. the address of input A's own
 * remote, so picking up that remote also picks its source. Only works if that remote uses
 * the same protocol. Leave undefined until recorded. */
/* #define INPUT_A_ADDR   0x0000 */

/* --- switch side ---------------------------------------------------------------------- */

/* Two sources, on inputs A and B of the switch. Only those two are ever connected, and the
 * switch never rests on an input without a source, so two LED senses suffice: input A's LED
 * on sense 0, input B's LED on sense 1 (PB0/PB1 on the ATtiny, D4/D5 on the Nano). These are
 * the read_input() results; the trigger toggles between them, odd states go to A. */
#define INPUT_A       1      /* sense 0 lit */
#define INPUT_B       2      /* sense 1 lit */

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
