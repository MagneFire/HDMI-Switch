/* IR-controlled HDMI switch.
 *
 * A pin-change interrupt decodes NEC frames from a TSOP38238 (both edges, mark/space pairs).
 * The main loop waits for the toggle key, reads which of the switch's three LEDs is lit,
 * and pulls the switch's own button low until the wanted LED comes on.
 *
 * Builds for ATmega328P (prototype) and ATtiny13A (final); see hal.h and Makefile.
 */
#include "config.h"
#include "hal.h"

/* --- NEC timing, in Timer0 ticks ------------------------------------------------------ */

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

_Static_assert(US_TO_TICKS(11250) < 65535u, "lead mark must fit the 16-bit tick counter");
_Static_assert(BIT_MIN > 0, "tick resolution too coarse");
_Static_assert(MS_TO_OVF(RESAMPLE_MS) > 0, "overflow clock too coarse");

#define IN(v, lo, hi) ((v) >= (lo) && (v) <= (hi))
#define NOT_IN_FRAME  0xFF

volatile uint16_t hal_ovf;

ISR(OVF_VECT)
{
    hal_ovf++;
}

/* --- decoder (ISR context only) ------------------------------------------------------- */

static uint16_t t_fall, t_rise;
static uint8_t nbits = NOT_IN_FRAME;
static uint8_t bitmask;
static uint8_t data[4];

static volatile uint8_t ir_ready;
static volatile uint16_t ir_addr;
static volatile uint8_t ir_cmd;

static inline void frame_start(void)
{
    nbits = 0;
    bitmask = 1;
    data[0] = data[1] = data[2] = data[3] = 0;
}

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
        data[nbits >> 3] |= bitmask;
    } else if (!IN(space, BIT_MIN, BIT_MAX)) {
        nbits = NOT_IN_FRAME;
        return;
    }
    bitmask <<= 1;
    if (!bitmask)
        bitmask = 1;

    if (++nbits == 32) {
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

/* --- debug trace (ATmega328P only) ---------------------------------------------------- */

#if defined(DEBUG) && defined(__AVR_ATmega328P__)
static void uart_putc(char c)
{
    while (!(UCSR0A & _BV(UDRE0)))
        ;
    UDR0 = (uint8_t)c;
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

static void debug_init(void)
{
    UCSR0A = _BV(U2X0);
    UBRR0 = (uint16_t)(F_CPU / 8UL / 115200UL - 1);  /* 16 -> 117.6 kbaud, 2 % off */
    UCSR0B = _BV(TXEN0);
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
#define debug_init()
#define debug_code(a, c)        ((void)0)
#define debug_state(t, c, g)    ((void)0)
#endif

/* --- switch state --------------------------------------------------------------------- */

/* 1..3 when exactly one LED is on, 0 otherwise */
static uint8_t read_input(void)
{
    uint8_t v = (uint8_t)((LED_PIN >> LED_SHIFT) & 7);
#if LED_ACTIVE_LOW
    v = (uint8_t)(~v & 7);
#endif
    if (v == 1) return 1;
    if (v == 2) return 2;
    if (v == 4) return 3;
    return 0;
}

static uint8_t read_input_settled(void)
{
    uint8_t v = read_input();
    if (!v) {
        delay_ovf(MS_TO_OVF(RESAMPLE_MS));
        v = read_input();
    }
    return v;
}

int main(void)
{
    uint16_t last_code = 0;
    uint16_t addr;
    uint8_t cmd, cur, target, presses;

    hal_init();
    debug_init();

    for (;;) {
        if (!ir_ready)
            continue;
        cli();
        ir_ready = 0;
        addr = ir_addr;
        cmd = ir_cmd;
        sei();
        debug_code(addr, cmd);

        if ((uint16_t)(now_ovf() - last_code) < MS_TO_OVF(DEBOUNCE_MS)) {
            last_code = now_ovf();      /* held key or double decode: the key must be */
            continue;                   /* released for DEBOUNCE_MS before the next flip */
        }

        cur = read_input_settled();
        if (addr == TOGGLE_ADDR && cmd == TOGGLE_CMD) {
            target = (cur == INPUT_KPN) ? INPUT_GOOGLE : INPUT_KPN;
#ifdef BOX_ADDR
        } else if (addr == BOX_ADDR) {
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
        last_code = now_ovf();          /* debounce counted from the end of the presses */
    }
}
