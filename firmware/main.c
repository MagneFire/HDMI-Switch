/* IR-controlled HDMI switch.
 *
 * A pin-change interrupt decodes IR frames from a TSOP receiver: NEC (mark/space pairs on
 * the falling edges) or Sony SIRC (mark widths on the rising edges), chosen by IR_PROTOCOL.
 * The main loop groups frames into presses, waits for the trigger (single press or double
 * tap), reads which of the switch's LEDs is lit, and pulls the switch's own button low
 * until the wanted LED comes on.
 *
 * Builds for ATmega328P (prototype) and ATtiny13A (final); see hal.h and Makefile.
 */
#include "config.h"
#include "hal.h"

#define IN(v, lo, hi) ((v) >= (lo) && (v) <= (hi))
#define NOT_IN_FRAME  0xFF

_Static_assert(US_TO_TICKS(11250) < 65535u, "lead mark must fit the 16-bit tick counter");
_Static_assert(US_TO_TICKS(350) > 0, "tick resolution too coarse");
_Static_assert(MS_TO_OVF(RESAMPLE_MS) > 0, "overflow clock too coarse");

volatile uint16_t hal_ovf;

ISR(OVF_VECT)
{
    hal_ovf++;
}

/* --- decoder state (ISR context only) ------------------------------------------------- */

static uint16_t t_fall, t_rise;
static uint8_t nbits = NOT_IN_FRAME;
static uint8_t bitmask;
static uint8_t data[4];

#ifdef DEBUG
#define DBG_RAW 42                      /* 21 pairs: a 20-bit SIRC frame plus header */
static volatile uint16_t dbg_raw[DBG_RAW], dbg_edges;
static volatile uint8_t dbg_n;
#define DBG_CAPTURE(a, b)                                       \
    do {                                                        \
        dbg_edges++;                                            \
        if (dbg_n < DBG_RAW) {                                  \
            dbg_raw[dbg_n++] = (a);                             \
            dbg_raw[dbg_n++] = (b);                             \
        }                                                       \
    } while (0)
#else
#define DBG_CAPTURE(a, b) ((void)0)
#endif

static volatile uint8_t ir_ready;
static volatile uint16_t ir_addr;
static volatile uint8_t ir_cmd;

static inline void frame_start(void)
{
    nbits = 0;
    bitmask = 1;
    data[0] = data[1] = data[2] = data[3] = 0;
}

static inline void store_bit(uint8_t one)
{
    if (one)
        data[nbits >> 3] |= bitmask;
    bitmask <<= 1;
    if (!bitmask)
        bitmask = 1;
    nbits++;
}

#if IR_PROTOCOL == IR_NEC

/* NEC: 9 ms lead mark, 4.5 ms space (2.25 ms for a repeat frame), then 32 bits LSB first as
 * 560 us marks with 560 us (0) or 1690 us (1) spaces: address, ~address, command, ~command.
 * Decoded on the falling edges, where a complete mark/space pair is known. */

#define LEAD_MARK_MIN   US_TO_TICKS(6750)   /*  9000 us -25 % */
#define LEAD_MARK_MAX   US_TO_TICKS(11250)
#define LEAD_SPACE_MIN  US_TO_TICKS(3375)   /*  4500 us */
#define LEAD_SPACE_MAX  US_TO_TICKS(5625)
#define BIT_MIN         US_TO_TICKS(350)    /*   560 us mark, or space of a 0 (TSOP stretch) */
#define BIT_MAX         US_TO_TICKS(800)
#define ONE_MIN         US_TO_TICKS(1200)   /*  1690 us space of a 1 */
#define ONE_MAX         US_TO_TICKS(2200)
#ifdef PROTO_SAMSUNG
#define SAM_MARK_MIN    US_TO_TICKS(3375)   /*  4500 us header mark */
#define SAM_MARK_MAX    US_TO_TICKS(5625)
#endif

ISR(PCINT_VECT)
{
    uint16_t now = tick();

    if (IR_PIN & _BV(IR_BIT)) {         /* rising edge: mark ended */
        t_rise = now;
        return;
    }

    /* falling edge: one mark/space pair is complete */
    uint16_t mark = t_rise - t_fall;
    uint16_t space = now - t_rise;
    t_fall = now;
    DBG_CAPTURE(mark, space);

    if (IN(mark, LEAD_MARK_MIN, LEAD_MARK_MAX)) {
        if (IN(space, LEAD_SPACE_MIN, LEAD_SPACE_MAX))
            frame_start();
        else
            nbits = NOT_IN_FRAME;       /* repeat frame (2.25 ms space): one event per press */
        return;
    }
#ifdef PROTO_SAMSUNG
    if (IN(mark, SAM_MARK_MIN, SAM_MARK_MAX) && IN(space, LEAD_SPACE_MIN, LEAD_SPACE_MAX)) {
        frame_start();
        return;
    }
#endif
    if (nbits >= 32)
        return;                         /* not inside a frame */

    if (!IN(mark, BIT_MIN, BIT_MAX)) {
        nbits = NOT_IN_FRAME;
        return;
    }
    if (IN(space, ONE_MIN, ONE_MAX)) {
        store_bit(1);
    } else if (IN(space, BIT_MIN, BIT_MAX)) {
        store_bit(0);
    } else {
        nbits = NOT_IN_FRAME;
        return;
    }

    if (nbits == 32) {
        if (data[2] == (uint8_t)~data[3]) {
            /* standard NEC sends addr, ~addr: report 8 bits, like IRremote does */
            ir_addr = (data[1] == (uint8_t)~data[0]) ? data[0]
                                                     : (uint16_t)(data[0] | (data[1] << 8));
            ir_cmd = data[2];
            ir_ready = 1;
        }
        nbits = NOT_IN_FRAME;
    }
}

#elif IR_PROTOCOL == IR_SIRC

/* Sony SIRC: 2.4 ms header mark, then SIRC_BITS bits LSB first as 1.2 ms (1) or 0.6 ms (0)
 * marks separated by 0.6 ms spaces: 7-bit command, then the address (5 bits for 12-bit
 * frames, 8 for 15, 5 + 8 extended for 20). Each press sends the frame three times, 45 ms
 * apart. Decoded on the rising edges, where the mark width is known; the frame is complete
 * with its last mark, no trailing gap needed. */

#define HDR_MIN         US_TO_TICKS(1800)   /*  2400 us */
#define HDR_MAX         US_TO_TICKS(3000)
#define ZERO_MIN        US_TO_TICKS(350)    /*   600 us */
#define ZERO_MAX        US_TO_TICKS(850)
#define ONE_MIN         US_TO_TICKS(900)    /*  1200 us */
#define ONE_MAX         US_TO_TICKS(1500)
#define SPACE_MIN       US_TO_TICKS(300)    /*   600 us, shortened by the TSOP's mark stretch */
#define SPACE_MAX       US_TO_TICKS(900)

ISR(PCINT_VECT)
{
    uint16_t now = tick();

    if (!(IR_PIN & _BV(IR_BIT))) {      /* falling edge: mark starts */
        t_fall = now;
        return;
    }

    /* rising edge: the mark and the space before it are known */
    uint16_t mark = now - t_fall;
    uint16_t space = t_fall - t_rise;
    t_rise = now;
    DBG_CAPTURE(space, mark);

    if (IN(mark, HDR_MIN, HDR_MAX)) {
        frame_start();
        return;
    }
    if (nbits >= SIRC_BITS)
        return;                         /* not inside a frame */

    if (!IN(space, SPACE_MIN, SPACE_MAX)) {
        nbits = NOT_IN_FRAME;
        return;
    }
    if (IN(mark, ONE_MIN, ONE_MAX)) {
        store_bit(1);
    } else if (IN(mark, ZERO_MIN, ZERO_MAX)) {
        store_bit(0);
    } else {
        nbits = NOT_IN_FRAME;
        return;
    }

    if (nbits == SIRC_BITS) {
        /* command = bits 0..6, address = the rest, like IRremote reports Sony */
        ir_cmd = (uint8_t)(data[0] & 0x7F);
        ir_addr = (uint16_t)(((uint16_t)(data[1] | (data[2] << 8)) << 1) | (data[0] >> 7));
        ir_ready = 1;
        nbits = NOT_IN_FRAME;
    }
}

#else
#error "IR_PROTOCOL must be IR_NEC or IR_SIRC"
#endif

/* --- debug trace (ATmega328P only) ---------------------------------------------------- */

#if defined(DEBUG) && defined(__AVR_ATmega328P__)
static void uart_raw(char c)
{
    while (!(UCSR0A & _BV(UDRE0)))
        ;
    UDR0 = (uint8_t)c;
}

static void uart_putc(char c)       /* terminals such as tio expect CR LF */
{
    if (c == '\n')
        uart_raw('\r');
    uart_raw(c);
}

static void uart_hex(uint8_t v)
{
    static const char digits[] = "0123456789ABCDEF";
    uart_putc(digits[v >> 4]);
    uart_putc(digits[v & 15]);
}

static void uart_str(const char *s)
{
    while (*s)
        uart_putc(*s++);
}

static void uart_dec(uint16_t v)
{
    char buf[6];
    uint8_t i = 0;
    do {
        buf[i++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (i)
        uart_putc(buf[--i]);
}

/* ticks -> microseconds, for the trace only */
#define TICKS_TO_US(t) ((uint16_t)((uint32_t)(t) * 1000000UL / TICKS_PER_SEC))

static void debug_init(void)
{
    UCSR0A = _BV(U2X0);
    UBRR0 = (uint16_t)(F_CPU / 8UL / 115200UL - 1);  /* 16 -> 117.6 kbaud, 2 % off */
    UCSR0B = _BV(TXEN0);
    uart_str("boot pin=");
    uart_putc((IR_PIN & _BV(IR_BIT)) ? '1' : '0');
    uart_putc('\n');
}

static void debug_poll(void)
{
    static uint16_t last_status;
    uint16_t now = now_ovf();

    if ((uint16_t)(now - last_status) >= MS_TO_OVF(1000)) {   /* live status line */
        last_status = now;
        uart_str("pin=");
        uart_putc((IR_PIN & _BV(IR_BIT)) ? '1' : '0');
        uart_str(" edges=");
        uart_dec(dbg_edges);
        uart_putc('\n');
    }
    if (dbg_n >= DBG_RAW) {             /* raw trace in us, oldest first */
        uint8_t i;
#if IR_PROTOCOL == IR_NEC
        uart_str("raw mark,space:");
#else
        uart_str("raw space,mark:");
#endif
        for (i = 0; i < DBG_RAW; i += 2) {
            uart_putc(' ');
            uart_dec(TICKS_TO_US(dbg_raw[i]));
            uart_putc(',');
            uart_dec(TICKS_TO_US(dbg_raw[i + 1]));
        }
        uart_putc('\n');
        dbg_n = 0;
    }
}

static void debug_code(uint16_t addr, uint8_t cmd)
{
    uart_str("A=");
    uart_hex((uint8_t)(addr >> 8));
    uart_hex((uint8_t)addr);
    uart_str(" C=");
    uart_hex(cmd);
    uart_putc('\n');
}

static void debug_state(const char *tag, uint8_t cur, uint8_t target)
{
    uart_str(tag);
    uart_str(" cur=");
    uart_putc((char)('0' + cur));
    uart_str(" tgt=");
    uart_putc((char)('0' + target));
    uart_putc('\n');
}
#else
#define debug_init()            ((void)0)
#define debug_poll()            ((void)0)
#define debug_code(a, c)        ((void)0)
#define debug_state(t, c, g)    ((void)0)
#endif

/* --- switch state --------------------------------------------------------------------- */

/* INPUT_KPN or INPUT_GOOGLE when exactly one sensed LED is on, 0 otherwise */
static uint8_t read_input(void)
{
    uint8_t v = (uint8_t)((LED_PIN >> LED_SHIFT) & 3);
#if LED_ACTIVE_LOW
    v = (uint8_t)(~v & 3);
#endif
    return v == 3 ? 0 : v;
}

/* The switch blanks all LEDs briefly after a press: keep sampling until one is lit, or give
 * up after RESAMPLE_MAX tries. */
static uint8_t read_input_settled(void)
{
    uint8_t v, tries = RESAMPLE_MAX;
    while (!(v = read_input()) && --tries)
        delay_ovf(MS_TO_OVF(RESAMPLE_MS));
    return v;
}

int main(void)
{
    uint16_t last_frame = 0;        /* time of the previous decoded frame */
    uint16_t last_action = 0;       /* end of the previous press sequence */
#ifdef DOUBLE_TAP_MS
    uint16_t last_tap = 0;
    uint8_t armed = 0;              /* first tap seen, waiting for the second */
#endif
    uint16_t now, addr;
    uint8_t cmd, cur, target, presses;

    hal_init();
    debug_init();

    for (;;) {
        debug_poll();
        if (!ir_ready)
            continue;
        cli();
        ir_ready = 0;
        addr = ir_addr;
        cmd = ir_cmd;
        sei();
        debug_code(addr, cmd);

        /* frames in quick succession are the same key held down: one press */
        now = now_ovf();
        if ((uint16_t)(now - last_frame) < MS_TO_OVF(PRESS_GAP_MS)) {
            last_frame = now;
            continue;
        }
        last_frame = now;

        if ((uint16_t)(now - last_action) < MS_TO_OVF(LOCKOUT_MS))
            continue;                   /* too soon after a switch action */

        if (addr == TOGGLE_ADDR && cmd == TOGGLE_CMD) {
#ifdef DOUBLE_TAP_MS
            if (!armed || (uint16_t)(now - last_tap) > MS_TO_OVF(DOUBLE_TAP_MS)) {
                armed = 1;              /* first tap: wait for the second */
                last_tap = now;
                continue;
            }
            armed = 0;
#endif
            cur = read_input_settled();
            target = (cur == INPUT_KPN) ? INPUT_GOOGLE : INPUT_KPN;
#ifdef BOX_ADDR
        } else if (addr == BOX_ADDR) {
            cur = read_input_settled();
            target = INPUT_KPN;
#endif
        } else {
            continue;
        }
        debug_state("go", cur, target);

        for (presses = 0; presses < MAX_PRESSES && cur != target; presses++) {
            press();
            delay_ovf(MS_TO_OVF(PRESS_MS));
            release();
            delay_ovf(MS_TO_OVF(SETTLE_MS));
            cur = read_input_settled();
        }
        debug_state(cur == target ? "ok" : "fail", cur, target);
        last_action = now_ovf();
        last_frame = last_action;       /* frames decoded during the presses: same press */
    }
}
