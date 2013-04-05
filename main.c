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
#include <avr/pgmspace.h>

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

// Define the amount of time (in mS) that the coil should
// be energised for a 'tick'.  This should be as low as 
// possible but varies between clock modules...
#define ENERGISE_TIME 30

// 128 timing values for 32 seconds
// (4 possible movements per second * 32 seconds = 128)
const uint8_t timingSequence[128] PROGMEM = {
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
const uint8_t timingRandomisation[15] PROGMEM = {
  0, 6, 8, 2, 10, 3, 4, 1, 7, 9, 5, 3, 8, 5, 9
  };  

volatile uint8_t timingPosition = 0;
volatile uint8_t randomisationPosition = 0;

// Global for storing the currently required polarity
volatile uint8_t polarity = 0;

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
  PORTB |= (1<< X1_BIT);
  PORTB |= (1<< X2_BIT);
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

int main(void) {
  // Enable timer 1 with 32 prescaler.
  // This gives 1/4 second between each overflow
  // (32 * 256 / 32768 clocks = 1/4 second)
  sbi(TCCR1, CS12);
  sbi(TCCR1, CS11);
  sbi(TIMSK, TOIE1); 
  sei();
	
	DDRB = (1<<X1_BIT)|(1<< X2_BIT);
  reset();

	PORTB |= (1<<PB2); // Turn on pull-up on unused pin
	
  set_sleep_mode(SLEEP_MODE_IDLE);

  while(1)
  {
    if (pgm_read_byte(&timingSequence[timingPosition]) == 1)
    {
      // Wait for the randomisation amount
      for (unsigned char delay = 0; delay < pgm_read_byte(&timingRandomisation[randomisationPosition]); delay++)
        _delay_ms(5);
        
      randomisationPosition++;
      if (randomisationPosition == 15) randomisationPosition = 0; 
        
      pulseClock();
    } 
    
    timingPosition++;
    if (timingPosition == 128) timingPosition = 0;

    sleep_mode(); // system sleeps here
  } 
}

// Timer 1 interrupt (will wake system from idle sleep mode)
ISR(TIMER1_OVF_vect) {
  _NOP();
}
