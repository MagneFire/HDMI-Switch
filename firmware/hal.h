/* Pin map, Timer0 time base and pin-change interrupt for the two targets.
 *
 * ATmega328P (Nano/Uno prototype)      ATtiny13A (final, same as ATtiny85)
 *   PD2  IR receiver OUT (PCINT18)       PB3  IR receiver OUT (PCINT3)
 *   PD3  switch button, open-drain       PB4  switch button, open-drain
 *   PD4..PD6  LED 1..3 sense             PB0..PB2  LED 1..3 sense (ISP lines, inputs)
 *
 * Timer0 runs free with prescaler /64 on both: 4 us/tick on the Uno, 6.67 us/tick on the 13A.
 * The overflow counter gives a coarse clock (1.02 ms / 1.71 ms per count) for the delays.
 */
#ifndef HAL_H
#define HAL_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#if defined(__AVR_ATtiny13A__) || defined(__AVR_ATtiny13__) || defined(__AVR_ATtiny85__)
#  define IR_PIN      PINB
#  define IR_PORT     PORTB
#  define IR_BIT      PB3
#  define BTN_DDR     DDRB
#  define BTN_PORT    PORTB
#  define BTN_BIT     PB4
#  define LED_PIN     PINB
#  define LED_SHIFT   0
#  define PCINT_VECT  PCINT0_vect
#  define OVF_VECT    TIM0_OVF_vect
static inline void hal_pcint_init(void)
{
    PCMSK = _BV(PCINT3);
    GIMSK |= _BV(PCIE);
}
#elif defined(__AVR_ATmega328P__)
#  define IR_PIN      PIND
#  define IR_PORT     PORTD
#  define IR_BIT      PD2
#  define BTN_DDR     DDRD
#  define BTN_PORT    PORTD
#  define BTN_BIT     PD3
#  define LED_PIN     PIND
#  define LED_SHIFT   4
#  define PCINT_VECT  PCINT2_vect
#  define OVF_VECT    TIMER0_OVF_vect
static inline void hal_pcint_init(void)
{
    PCMSK2 = _BV(PCINT18);
    PCICR |= _BV(PCIE2);
}
#else
#  error "unsupported MCU: add a pin map in hal.h"
#endif

/* --- time base ------------------------------------------------------------------------ */

#define TICKS_PER_SEC   (F_CPU / 64UL)
#define OVF_PER_SEC     (F_CPU / 64UL / 256UL)
#define US_TO_TICKS(us) ((uint16_t)((us) * TICKS_PER_SEC / 1000000UL))
#define MS_TO_OVF(ms)   ((uint16_t)((ms) * OVF_PER_SEC / 1000UL))

extern volatile uint16_t hal_ovf;   /* Timer0 overflows since reset */

/* 16-bit tick counter. Only valid with interrupts disabled (i.e. inside an ISR). */
static inline uint16_t tick(void)
{
    uint8_t lo = TCNT0;
    uint8_t hi = (uint8_t)hal_ovf;
    if ((TIFR0 & _BV(TOV0)) && lo < 128)
        hi++;                       /* overflow pending, not yet counted */
    return ((uint16_t)hi << 8) | lo;
}

static inline uint16_t now_ovf(void)
{
    uint16_t v;
    cli();
    v = hal_ovf;
    sei();
    return v;
}

static inline void delay_ovf(uint16_t n)
{
    uint16_t start = now_ovf();
    while ((uint16_t)(now_ovf() - start) < n)
        ;
}

/* --- pins ----------------------------------------------------------------------------- */

static inline void press(void)      /* drive low */
{
    BTN_PORT &= (uint8_t)~_BV(BTN_BIT);
    BTN_DDR |= _BV(BTN_BIT);
}

static inline void release(void)    /* back to high impedance */
{
    BTN_DDR &= (uint8_t)~_BV(BTN_BIT);
}

static inline void hal_init(void)
{
    IR_PORT |= _BV(IR_BIT);         /* pull-up on the receiver line; TSOP is open-collector */
    release();                       /* button idle, LED senses stay plain inputs */
    TCCR0B = _BV(CS01) | _BV(CS00); /* Timer0 /64, normal mode */
    TIMSK0 = _BV(TOIE0);
    hal_pcint_init();
    sei();
}

#endif
