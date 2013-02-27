/************************************************************************
	main.c

    Vetinari Clock
    Copyright (C) 2011 Simon Inns

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

	Email: simon.inns@gmail.com

************************************************************************/

#ifndef MAIN_C
#define MAIN_C

// Global includes
#include <htc.h>

// NOTE: This code uses the 9.82 header files - it WILL NOT compile
// on older versions of the HiTech PIC C compiler!

// Fuse configuration for the PIC12F683
__CONFIG(FCMEN_OFF & IESO_OFF & BOREN_OFF & CP_OFF & CPD_OFF & MCLRE_ON &
	PWRTE_OFF & WDTE_OFF & FOSC_LP);

// Define oscillator frequency (32.768 Khz clock crystal)
#define _XTAL_FREQ 32768

// Hardware mapping definitions
#define CLK_OUT0	GP1
#define CLK_OUT1	GP2
#define BUTTON0		GP0

// Useful definitions
#define OFF		0
#define ON		1
#define FALSE	0
#define TRUE	1

// Define the amount of time (in mS) that the coil should
// be energised for a 'tick'.  This should be as low as 
// possible but varies between clock modules...
#define ENERGISE_TIME	30

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

// Send a pulse to the clock module
void pulseClock(void)
{
	if (polarity == 0)
	{
		CLK_OUT0 = 1;
		CLK_OUT1 = 0;
		
		// Wait x mS
		__delay_ms(ENERGISE_TIME);
		
		CLK_OUT0 = 0;
		CLK_OUT1 = 0;
		
		// Switch polarity
		polarity = 1;
	}
	else
	{
		CLK_OUT0 = 0;
		CLK_OUT1 = 1;
		
		// Wait x mS
		__delay_ms(ENERGISE_TIME);
		
		CLK_OUT0 = 0;
		CLK_OUT1 = 0;
		
		// Switch polarity
		polarity = 0;
	}		
}	

// Main procedure
void main(void)
{
	// Set up the PIC12F683 IO pins
	GPIO   = 0b00000000;	// Set all pins to zero
	ANSEL  = 0b00000000;	// Disable analogue inputs
	TRISIO = 0b00000000;	// All pins are output
	WPU    = 0b00000000;	// Set weak pull-up off on all pins
	CMCON0 = 7;				// Disable comparator 0
	
	// Fosc is 32.768 hz so Fosc/4 is 8,192 timer ticks per second.
	// We will use a 1:8 prescaler which will mean the timer overflows
	// 4 times a second.
	//
	// We want the overflow to be at exactly 255 so we don't have to use
	// cycles reseting the timer.  This will mean (provided that the
	// interrupt lasts >.25 of a second) we will never lose a tick. Which
	// will mean the clock is as accurate as the crystal.

	TMR0 = 0;	// Clear timer0 register
	OPTION_REG = 0b000000010;	// Use internal cycle clock, assign the
								// prescaler to timer0 and set to 1:32 prescale
	T0IF = 0;	// Clear timer0 interrupt flag

	unsigned char counter = 0;

	while(1)
	{
		// Wait for the timer0 overflow flag to be set
		while(!T0IF);
		
		// Reset the timer0 interrupt flag
		T0IF = 0;
		
		if (timingSequence[counter] == 1)
		{
			// Wait for the randomisation amount
			for (unsigned char delay = 0; delay < timingRandomisation[randomisationPosition]; delay++)
				__delay_ms(5);
				
			randomisationPosition++;
			if (randomisationPosition == 15) randomisationPosition = 0;	
				
			pulseClock();
		}	
		
		counter++;
		if (counter == 128) counter = 0;
	}	
}

#endif