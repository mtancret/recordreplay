/**
 * Author(s): Matthew Tan Creti
 *            matthew.tancreti at gmail dot com
 */
#ifndef _TELOS_H
#define _TELOS_H

/******************************************************************************
 * Interrupts
 */

#define GIE (0x0008)
#define asmv(arg) __asm__ __volatile__(arg)
#define SPLHIGH(sr)\
        asmv("mov r2, %0" : "=r" (sr));\
        asmv("bic %0, r2" : : "i" (GIE));\
        sr &= GIE
#define SPLX(sr)\
        asmv("bis %0, r2" : : "r" (sr))

/******************************************************************************
 * Watchdog
 */
#define TSCRIBE_DISABLE_WATCHDOG() WDTCTL = WDTPW | WDTHOLD

/******************************************************************************
 * LEDS
 */
#define TSCRIBE_RED_LED (1<<4)
#define TSCRIBE_GREEN_LED (1<<5)
#define TSCRIBE_BLUE_LED (1<<6)
#define TSCRIBE_ALL_LEDS (TSCRIBE_RED_LED | TSCRIBE_GREEN_LED | TSCRIBE_BLUE_LED)
#define TSCRIBE_INIT_LEDS() P5DIR |= TSCRIBE_ALL_LEDS
#define TSCRIBE_LED_OFF(LED) P5OUT |= LED
#define TSCRIBE_LED_ON(LED) P5OUT &= ~LED
#define TSCRIBE_TOGGLE_LED(LED) P5OUT ^= LED

/******************************************************************************
 * User button
 */

#define TSCRIBE_USER_BUTTON_PRESSED() ((P2IN & (1<<7)) == 0)

/******************************************************************************
 * Simulation
 */

#endif
