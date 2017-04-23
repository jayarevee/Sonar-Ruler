
//  Description: Input capture on P1.4 (TA1.0)
//  If button press duration > 1 sec, then generate PWM of 75% on P2.4
//  else generate a 25% duty cycle on P2.4.

#include <msp430.h>
#include <stdint.h>

#define PERIOD 327				/* 100.24Hz = 32768Hz/PERIOD */

void main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer
	PM5CTL0 &= ~LOCKLPM5;			// Unlock ports from power manager
	__delay_cycles(200000);			// Delay for crystal stabilization

	// Timer capture
	P1DIR   &= ~BIT4;				// Set P1.4 as input
	P1REN   |=  BIT4;				// Enable pull-up/down resistor on P1.4
	P1OUT   |=  BIT4;				// Set P1.4 resistor as pull-up
	P1SEL0  |=  BIT4;				// Connect P1.4 to timer capture (device 3)
	P1SEL1  |=  BIT4;				// ... see page 94 in the datasheet (slas789b)
	// Falling edge, synchronized capture, select source 0 (external pin), enable capture/interrupt
	TA1CCTL0 = CM_3 | SCS | CCIS_0 | CAP | CCIE;
	// ACLK/2, continuous up mode, clear timer
	TA1CTL   = TASSEL__ACLK | MC__CONTINUOUS | ID__2 | TACLR;

	// PWM from lab 4
	P2DIR  |=  BIT4;				// Set P2.4 as output
	P2SEL0 |=  BIT4;				// Connect P2.4 to timer output (device 1)
	P2SEL1 &= ~BIT4;				// ... see page 97 in the datasheet (slas789b)
	TB0CCR0  = PERIOD - 1;			// PWM period
	TB0CCTL3 = OUTMOD_7;			// EQU3-reset/EQU0-set output
	// ACLK/2, up mode, clear timer
	TB0CTL   = TBSSEL__ACLK | MC__UP | ID__2 | TBCLR;
	
	__enable_interrupt();
	for (;;) __bis_SR_register(LPM0_bits | GIE);
}

static uint16_t last_cap = 0;		// Time when the last capture happened
#pragma vector = TIMER1_A0_VECTOR
__interrupt void button_timer(void) {
	uint16_t new_cap = TA1CCR0;		// Get the captured time
	uint16_t cap_diff = new_cap - last_cap; // Calculate the time pressed
	last_cap = new_cap;				// Store the captured time for next press

	if (TA1CCTL0 & CCI) {				// Change LED on rising edge only (release)
		if (cap_diff > 16384)			// 1 second is 16384 ticks (32768 Hz / 2)
			TB0CCR3 = 3 * PERIOD / 4;	// 75% duty cycle if pressed > than 1 sec
		else
			TB0CCR3 = PERIOD / 4;		// 25% duty cycle otherwise
	}
}
