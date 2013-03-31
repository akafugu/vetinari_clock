/*
 * Vetinari Clock
 * (C) 2011 Simon Inns
 *
 * avr-gcc port for Attiny25
 * (C) 2013 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include <stdbool.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define _NOP() do { __asm__ __volatile__ ("nop"); } while (0) 

#define X1_BIT PB0
#define X2_BIT PB1

#define ledPin PB0
#define GND_BIT PB1

volatile bool led_state = 0; // LED state on/off variable
volatile bool overflow;

// Define the amount of time (in mS) that the coil should
// be energised for a 'tick'.  This should be as low as 
// possible but varies between clock modules...
#define ENERGISE_TIME 30

// 64 timing values for 32 seconds
const unsigned char timingSequence[128] = {
  1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
  0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1,
  0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0,
  0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
  1, 1
  };
  
// The randomisation amount adds a delay before moving the 
// seconds hand to ensure the movement is not always on the 
// quarter second boundary.  The extra delay is in units of
// 5 mS.
const unsigned char timingRandomisation[15] = {
  0, 13, 16, 4, 20, 5, 8, 2, 14, 18, 3, 7, 15, 10, 17
  };  

unsigned char timingPosition = 0;
unsigned char randomisationPosition = 0;

// Global for storing the currently required polarity
unsigned char polarity = 0;

void tick(void)
{
  PORTB |= (1 << X1_BIT);
  PORTB &= ~(1 << X2_BIT);
}

void tock(void)
{
  PORTB |= (1 << X2_BIT);
  PORTB &= ~(1 << X1_BIT);
}

void reset(void)
{
  DDRB |= (1<< X1_BIT);
  DDRB |= (1<< X2_BIT);
}

// Send a pulse to the clock module
void pulseClock(void)
{
  if (polarity == 0) {
    tick();
    _delay_ms(ENERGISE_TIME);
    reset();
    polarity = 1;
  }
  else
  {
    tock();
    _delay_ms(ENERGISE_TIME);
    reset();
    polarity = 0;
  }   
} 

// random number seed (will give same sequence every time)
volatile uint32_t lfsr = 0xbeefcace;

uint32_t rand(void)
{
  // http://en.wikipedia.org/wiki/Linear_feedback_shift_register
  // Galois LFSR: taps: 32 31 29 1; characteristic polynomial: x^32 + x^31 + x^29 + x + 1 */
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
  return lfsr;
}

#define MODE_SKIP 0 // skip a tick
#define MODE_FAST 1 // tick twice

int main(void) {
  // Enable timer 1 with 32 prescaler.
  // This gives 1/4 second between each overflow
  // (32 * 256 / 32768 clocks = 1/4 second)
  sbi(TCCR1, CS12);
  sbi(TCCR1, CS11);
  //TCCR1 |= (1<<CS13);
  //TCCR1 |= (1<<CS12);
  sbi(TIMSK, TOIE1); 
  sei();
	
	DDRB = (1<<X1_BIT)|(1<< X2_BIT);

  sbi(DDRB, X2_BIT);
  cbi(PORTB, X2_BIT);

	PORTB |= (1<<PB2); // Turn on pull-up on unused pin
	
  uint8_t mode = MODE_SKIP;
  unsigned char counter = 0;
  uint8_t r;

  while(1)
  {
    counter++;
    r = rand() % 10;

    if (r == 0 && mode == MODE_FAST && counter == 2) {
      pulseClock(); // extra tick
      mode = MODE_SKIP;
    }
    else if (r == 0 && mode == MODE_SKIP && counter == 4) {
      // skip a tick
      mode = MODE_FAST;
      counter = 0;
    }
    else if (counter == 4) {
      pulseClock();
      counter = 0;
    }

    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode(); // system sleeps here   
  }
}

// Timer 1 interrupt (will wake system from idle sleep mode)
ISR(TIMER1_OVF_vect) {
  _NOP();
}
