/*
@file sys_init.c

@brief This function initializes the hardware.


@ Created by Matt Heins, HackMe Electronics, 2011
This file is part of Sprockit.

    Sprockit is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Sprockit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Sprockit.  If not, see <http://www.gnu.org/licenses/>
*/

#include <sprockit_main.h>
#include <io.h>
#include <interrupt.h>
#include <led_switch_handler.h>
#include <sys_init.h>
#include <uart.h>
#include <midi.h>


/*
sys_init()
Configures the microcontroller
Setting up ports, timers and interrupts
External crystal oscillator frequency is 18.432MHz chosen because this is an even multiple of 48000
*/
void sys_init(void)
{
	unsigned int temp;

	//PORTB Setup
	DDRB = 0xEF;//Port B Data Direction:  0,1,2,3,5 outputs : 4 input
	PORTB = 0xEF;//Initialize outputs high, no pull-up in

	//PORTC Setup
	DDRC = 0xFC;//Port C Data Direction: 0,1 inputs : 2-6 outputs
	PORTC = 0xFC;//Initialize ouptuts high: no pull-up in

	//PORTD Setup
	DDRD =  0xF2;//PORTD Data Direction:  0,2,3 inputs : 1,4,5,6,7 outputs
	PORTD = 0XFE;//Initialize outputs high : 2,3 pull-up
	
	/*External Interrupt*/
	EIMSK = 0xFF;//Interrupts 0 and 1 enable
	EICRA = 0x0A;//Falling edge interrupt for interrupts 1 and 0
	
	
	/*
	Timer2 Setup - 8 bit timer
	Timer 2 supplies the sample timing
	The clock is divided by 8, 18.432MHz/8 = 2.304MHz  
	Output sample frequency = 2.304MHz/(256-TCNT0)
	For Example:
	18.432MHz/48000 = 384, 384/8 = 48
	Sample frequency is stored in a global variable (un16sampleFrequency) to make it easy to change
	*/
 
	temp = 19660800/SAMPLE_FREQUENCY;//clock cycles per sample
	temp /= 8;//divided down by 8

	TCCR2B = 0x02;//clk/8 = 18.432MHz/8 = 2.304MHz
	OCR2A = (temp - 1);//set the timer output compare value - when the counter gets to this number
	      //it triggers an interrupt and the timer is reset 
	      // - 1 because the counter starts at 0!!!
	TCCR2A = 0x02;//waveform generation bits are set to normal mode - no ports are triggered
	TIMSK2 = 0x02;//Enable Timer0 Output Compare Interrupt

	/*
	Timer1 Setup - 16 bit timer
	PWM Generator
	Output A is the voltage-controlled amplifier control voltage
	Output B is the main audio output
	*/
	
	TCCR1A = (1<<COM1A1)|(1<<COM1B1)|(1<<WGM10);//Set for fast PWM,8bit, set bit at bottom, clear when counter equals compare value
	TCCR1B = (1<<WGM12)|(1<<CS10);//Set for fast PWM, no prescaler
	OCR1BL = 0;//initially set the compare to 0
	OCR1AL = 0;

	/*
	Timer0 Setup - 8 bit timer
	This generates the slow interrupt for events in the main loop
	*/
	TCCR0B = (1<<CS01)|(1<<CS00);//clk/64 = 19.6608MHz/64 = 307,200
	OCR0A = 95;// /96 = 3200Hz
	TCCR0A = 0x02; //waveform generation bits are set to normal mode - no ports are triggered
	TIMSK0 = 0x02;//enable timer output compare interrupt
	
	/*
	Configure Analog to Digital Converter - Used for reading the pots
	The AD is left-justified down to 8 bits
	The AD needs to be prescaled to run at a maximum of 200kHz
	*/
	ADMUX = 0x20;//left-justify the result - 8 bit resolution, AD source is ADC0
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);//ADC Enable, Prescale 128 
 
	/*
	Configure SPI
	The SPI is interrupt driven, meaning we use the interrupt to know about the end of transmission.
	*/
	SPCR = (1<<SPE)|(1<<MSTR);

	SPSR = (1<<SPI2X);

	/*Initialize the UART for MIDI and initialize the MIDI state machine*/
	uart_init();
	midi_init();
	led_init();

}

