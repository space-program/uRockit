/* 
@file oscillator.c

@brief

This file contains the oscillator function which generates waveforms from wavetables and from 
resynthesizing wavetables to make for big fun. If you wanna, this is one of the most fun
places to play around because you can get truly creative and get really new sounds. Cheers!

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
#include <pgmspace.h>
#include <oscillator.h>
#include <wavetables.h>
#include <amp_adsr.h>

/*This oscillator lookup array changes oscillator 2 based on the setting for oscillator 1. It
also sets the oscillator mix between the two oscillators. This setting of oscillator 2 only
happens if the oscillator shape has not been */
const unsigned char AUC_OSCILLATOR_LUT [32][3] = {
{0,0,0,},//0
{1,0,0,},//1
{2,0,0,},//2
{3,0,0,},//3
{4,0,0,},//4
{5,0,0,},//5
{6,0,0,},//6
{0,0,127,},//7
{1,3,127,},//8
{1,0,127,},//9
{1,1,127,},//10
{2,1,127,},//11
{3,1,127,},//12
{4,1,127,},//13
{2,2,127,},//14
{3,2,127,},//15
{6,6,127,},//16
{7,7,127,},//17
{3,3,127,},//18
{4,4,127,},//19
{4,5,127,},//20
{5,7,127,},//21
{14,14,127,},//22
{15,7,127,},//23
{10,10,127,},//24
{11,6,127,},//25
{12,10,127,},//26
{13,13,127,},//27
{6,9,127,},//28
{1,14,127,},//29
{1,15,32,},//30
{15,15,127}};//31

/*
Function: decode_oscillator_waveshape
Takes: 
g_setting *p_global_setting - The global synthesizer setting array.
unsigned char ucwaveshape - This tells the function the setting of the oscillator pot.
      
Returns: Nothing.

This function takes the setting of the oscillator pot knob and decodes it to set the waveshape
for each oscillator and the oscillator mix parameter.
*/
void
decode_oscillator_waveshape(volatile g_setting *p_global_setting, unsigned char ucwaveshape)
{
	ucwaveshape = ucwaveshape >> 3;//32 waveshapes
	
	p_global_setting->auc_synth_params[OSC_1_WAVESHAPE] = AUC_OSCILLATOR_LUT[ucwaveshape][OSCILLATOR_1];
	
	if(p_global_setting->auc_parameter_source[OSC_2_WAVESHAPE] == SOURCE_AD)
	{
		p_global_setting->auc_synth_params[OSC_2_WAVESHAPE] = AUC_OSCILLATOR_LUT[ucwaveshape][OSCILLATOR_2];
	}
	
	if(p_global_setting->auc_parameter_source[OSC_MIX] == SOURCE_AD)
	{
		p_global_setting->auc_synth_params[OSC_MIX] = AUC_OSCILLATOR_LUT[ucwaveshape][OSCILLATOR_MIX];
	}
}

/*
Function: oscillator
Takes: unsigned char ucwaveshape - This tells the function which waveshape to generate
       unsigned int unsample_reference - This tells the function where we are in the wave cycle

Returns: unsigned char - An eight bit unsigned sample value.

This function takes a waveshape variable, a sample reference (where we are in the cycle), and a frequency 
and returns an appropriate 8 bit value for that point in the cycle.  It may take a wavetable directly, or 
add some together or some other math function to make some new time-varying waveshape.

*/
unsigned char 
oscillator(unsigned char uc_waveshape, unsigned int un_sample_reference, unsigned char uc_frequency){
	
	unsigned char	uc_temp,
					uc_interpolate_sample_1,
					uc_interpolate_sample_2,
					uc_interpolate_reference,
					uc_sample = 0,
					uc_morph_sample_1 = 0,
					uc_morph_sample_2 = 0,
					lfsr_bit,
					uc_sample_index,
					uc_table_modulus,
					uc_reverse_sample_index,
					uc_pitch_shift;
					
	unsigned int	un_temp,
					un_temp2,
					un_sample_calc = 0;
					
	signed int sn_temp;
					
	static unsigned char uc_morph_timer,
						 uc_morph_index,
						 uc_morph_state,
						 uc_phase_shifter,
						 uc_phase_shift_timer;
						 
	static unsigned int un_morph_index,
						un_morph_timer,
						lfsr = 0xACE1; 
	
	
	/*Some of the code in this file has not been space optimized. I did this to make it easier for me to develop. Work
	can be done to decrease the size of this code by creating functions to handle wavetable blending and morphing.*/

	/*The morph timer is used to control the change between different waveshapes. Each tick of the morph timer
	is one sample period. In this case, one morph timer increment is 1/32768 = 30 microseconds.*/
	
	if(g_uc_oscillator_midi_sync_flag == 1)
	{
		g_uc_oscillator_midi_sync_flag = 0;
		uc_morph_state = 0;
		
		un_morph_index = 0;
		uc_morph_timer = 0;
		
		uc_pitch_shift = 0;
	
		if(uc_waveshape !=  MORPH_7)
		{
			uc_morph_index = 0;
		}
	}


	/*Wavetable Blending Explained:
	A bunch of the wavetables below have blending going on. Rather than explain it every time. Here's
	what's going on.
	The wavetables are set up with 32 tables for 128 notes, that is 4 notes per table.
	In order to smooth the transition from table to table, we need to blend the tables together.
	We make a weighted average of 4 samples. 
	1st note = half current table + half table below/2
	2nd note = 3* current table + 1 from table below /4
	3rd note = 3* current table + 1 from table above
	4th note = half current table + half from table above /2
	Not so hard, n'est-ce pas?
	*/
					
	//switch statment for different waveshapes
	switch(uc_waveshape)
	{
		//Sin values are in a lookup table generated by calculating one cycle of a sine wave 
		case SIN:
			
			uc_temp = un_sample_reference >> 7;
			
			//To interpolate, we need the remainder which is what's discarded above.
			uc_interpolate_reference = un_sample_reference & 0x7F;
			
			//We need the first sample.
			uc_interpolate_sample_1 = G_AUC_SIN_LUT[uc_temp];

			//We need to get the next sample.
			uc_temp++;

			uc_interpolate_sample_2 = G_AUC_SIN_LUT[uc_temp];
						
			uc_sample = linear_interpolate(uc_interpolate_reference, uc_interpolate_sample_1, uc_interpolate_sample_2);
		
		break;

		case SQUARE:
	
			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256
			
			uc_sample = calculate_square(uc_sample_index,uc_frequency,127);
			
		break;
							
		case RAMP:
			
			uc_table_modulus = uc_frequency%4;
			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256
			
			/*This operation blends wavetables to minimize moving aliasing as the keyboard is played*/
			switch(uc_table_modulus)
			{
				case 0:

					un_sample_calc = pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					uc_frequency--;
					un_sample_calc += pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc >>= 1;

				break;

				case 1:

					un_sample_calc = pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc *= 3;
					uc_frequency--;
					un_sample_calc += pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc >>= 2;

				break;

				case 2:

					un_sample_calc = pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc *= 3;
					uc_frequency++;
					un_sample_calc += pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc >>= 2;

				break;

				case 3:
					un_sample_calc = pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					uc_frequency++;
					un_sample_calc += pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc >>= 1;

				break;

				default:
				
					un_sample_calc = 0;

				break;

			}
			
			uc_sample = (unsigned char) un_sample_calc;		
		
		break;

						
		case TRIANGLE:

			uc_table_modulus = uc_frequency%4;
			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256
			
			/*This operation blends wavetables to minimize moving aliasing as the keyboard is played*/
			switch(uc_table_modulus)
			{
				case 0:

					un_sample_calc = pgm_read_byte(&G_AUC_TRIANGLE_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					uc_frequency--;
					un_sample_calc += pgm_read_byte(&G_AUC_TRIANGLE_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc >>= 1;

				break;

				case 1:

					un_sample_calc = pgm_read_byte(&G_AUC_TRIANGLE_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc *= 3;
					uc_frequency--;
					un_sample_calc += pgm_read_byte(&G_AUC_TRIANGLE_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc >>= 2;

				break;

				case 2:

					un_sample_calc = pgm_read_byte(&G_AUC_TRIANGLE_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc *= 3;
					uc_frequency++;
					un_sample_calc += pgm_read_byte(&G_AUC_TRIANGLE_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc >>= 2;

				break;

				case 3:
					un_sample_calc = pgm_read_byte(&G_AUC_TRIANGLE_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					uc_frequency++;
					un_sample_calc += pgm_read_byte(&G_AUC_TRIANGLE_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
					un_sample_calc >>= 1;

				break;

				default:
				
					un_sample_calc = 0;

				break;

			}
			
			uc_sample = (unsigned char) un_sample_calc;	
				
		break;
					

		case MORPH_1:
			
			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256

			if(uc_morph_timer == 0)
			{
				uc_morph_index++;
				uc_morph_timer = 10;
			}

			uc_morph_timer--;
		
			uc_morph_sample_1 = calculate_square(uc_sample_index,uc_frequency,uc_morph_index);

			uc_sample_index += 127;
			
			uc_morph_sample_2 = pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);

			un_temp = uc_morph_sample_1*uc_morph_index;
			un_temp2 = uc_morph_sample_2*(255 - uc_morph_index);

			un_temp += un_temp2;

			uc_sample = un_temp >> 8;

		break;

		case MORPH_2:

			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256

			if(uc_morph_timer == 0)
			{
				uc_morph_index++;
				
				uc_morph_timer = MORPH_2_TIME_PERIOD;
			}

			uc_morph_timer--;

			uc_phase_shift_timer--;

			if(uc_phase_shift_timer == 0)
			{
				uc_phase_shifter++;
				
				uc_phase_shift_timer = PHASE_SHIFT_TIMER_2;
			}
			
			uc_morph_sample_1 = pgm_read_byte(&G_AUC_TRIANGLE_WAVETABLE_LUT[uc_frequency][uc_sample_index]);

			uc_sample_index += uc_phase_shifter;
			
			uc_morph_sample_2 = pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);

			un_temp = uc_morph_sample_1*uc_morph_index;
			un_temp2 = uc_morph_sample_2*(255 - uc_morph_index);

			un_temp += un_temp2;

			uc_sample = un_temp >> 8;

		break;

		case MORPH_3:
				
			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256
			
			if(uc_morph_timer == 0)
			{
				uc_morph_index++;
				uc_morph_timer = 50;
			}

			uc_morph_timer--;
			
			uc_reverse_sample_index = uc_sample_index - uc_morph_index;
			
			uc_temp = pgm_read_byte(&G_AUC_TRIANGLE_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
		
			sn_temp = uc_temp - pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_reverse_sample_index]);

			/*Now we'll have a positive or negative number. We have to center it around 127 and make sure
			that the sample is never going to be over 255 or less than 0.*/
			if(sn_temp > 127)
			{
				uc_sample = 255;
			}
			else if(sn_temp < -128)
			{
				uc_sample = 0;
			}
			else
			{
				uc_sample = 128 + sn_temp;
			}

		break;
		
		case MORPH_4:

			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256
		
			if(uc_morph_timer == 0)
			{
				if(uc_morph_state == 0)
				{
					uc_morph_index++;

					if(uc_morph_index == 255)
					{
						uc_morph_state = 1;
					}
				}
				else
				{
					uc_morph_index--;

					if(uc_morph_index == 0)
					{
						uc_morph_state = 0;
					}
				}

				uc_morph_timer = 250;
			}

			uc_morph_timer--;
			
			uc_sample = calculate_square(uc_sample_index,uc_frequency,uc_morph_index);

		break;
		
		case MORPH_5:
		
			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256

			if(uc_morph_timer == 0)
			{
				un_morph_index++;
				uc_morph_timer = 10;
			}

			uc_morph_timer--;
			
			/*First enveloped oscillator*/
			if(un_morph_index < 255)
			{
				un_sample_calc = G_AUC_SIN_LUT[uc_sample_index];
				un_sample_calc *= G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[un_morph_index];
				un_sample_calc >>= 8;
				uc_morph_sample_1 = (unsigned char) un_sample_calc;
			}				
			
			if(un_morph_index > 128 && un_morph_index < 383)
			{
				uc_temp = un_morph_index - 128;				
				
				un_sample_calc = calculate_square(uc_sample_index,uc_frequency,uc_morph_index);
				un_sample_calc *= 255 - G_AUC_SIN_LUT[uc_temp];
				un_sample_calc >>= 8;
				uc_morph_sample_2 = (unsigned char) un_sample_calc;
				
			}
			
			un_sample_calc = uc_morph_sample_1 + uc_morph_sample_2;
			un_sample_calc = un_sample_calc >> 1;
			uc_sample = (unsigned char) un_sample_calc;

			if(un_morph_index == 383)
			{
				un_morph_index = 0;
			}
		
		break;
		
		case MORPH_6:
		
			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256

			if(uc_morph_timer == 0)
			{
				un_morph_index++;
				uc_morph_timer = 50;
			}

			uc_morph_timer--;
			
			/*First enveloped oscillator*/
			if(un_morph_index < 255)
			{
				un_sample_calc = G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_sample_index];
				un_sample_calc *= G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[un_morph_index];
				un_sample_calc >>= 8;
				uc_morph_sample_1 = (unsigned char) un_sample_calc;
			}				
			
			if(un_morph_index > 128 && un_morph_index < 383)
			{
				uc_temp = un_morph_index - 128;
				un_sample_calc = calculate_square(uc_sample_index,uc_frequency,uc_morph_index);
				un_sample_calc *= 255 - G_AUC_SIN_LUT[uc_temp];
				un_sample_calc >>= 8;
				uc_morph_sample_2 = (unsigned char) un_sample_calc;
				
			}
			
			un_sample_calc = uc_morph_sample_1 + uc_morph_sample_2;
			un_sample_calc = un_sample_calc >> 1;
			uc_sample = (unsigned char) un_sample_calc;

			if(un_morph_index == 383)
			{
				un_morph_index = 0;
			}
		
		break;
		
		case MORPH_7:
		
			/*This morphing waveshape is a square wave with varying pulse width.*/
		
			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256
		
			/*This morphing waveshape is a square wave of varying pulse width*/
			if(uc_morph_timer == 0)
			{
				uc_morph_index++;
				uc_morph_timer = 25;
			}

			uc_morph_timer--;
			
			uc_sample = calculate_square(uc_sample_index,uc_frequency,uc_morph_index);
			
		break;
		
		case MORPH_8:
		
			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256
			
			if(un_morph_timer == 0)
			{
				uc_morph_index++;
				un_morph_timer = 4000;
				
			}
			
			un_morph_timer--;		
			uc_sample = calculate_square(uc_sample_index,uc_frequency,uc_morph_index);
			
			
		break;

		case MORPH_9:

			uc_frequency >>= 2;//Only 32 tables
			uc_sample_index = un_sample_reference>>7;//shift from 32768 to 256
				
			if(uc_morph_timer == 0)
			{
				uc_morph_index++;
				uc_morph_timer = 10;
			}

			uc_morph_timer--;
					
			uc_sample = calculate_square(uc_sample_index,uc_frequency,uc_morph_index);
		
		break;

		case HARD_SYNC:

			uc_frequency >>= 3;
			uc_sample_index = un_sample_reference>>8;
			uc_sample = calculate_square(uc_sample_index,uc_frequency,126);

		break;


		case NOISE:

			/*A pseudo random number is generated using a linear feedback shift register
			The polynomial expression used is: x^16 + x^14 + x^13 + x^11 + 1*/

			lfsr_bit = ((lfsr >> 15) ^ (lfsr >> 13) ^ (lfsr ^ 12) ^ (lfsr >> 10)) & 1;
			lfsr = (lfsr << 1) | (lfsr_bit); 
        
			uc_sample = (unsigned char) lfsr;

		break;
		
		case RAW_SQUARE:

			if(un_sample_reference > HALF_SAMPLE_MAX)
			{
				uc_sample = 255;
			}
			else
			{
				uc_sample = 0;
			}

		break;


		default:

			uc_temp = un_sample_reference >> 7;				
			uc_sample = G_AUC_SIN_LUT[uc_temp];
		
		break;

	}

 	return uc_sample;
				
}


/*
calculate_square
@brief This function calculates a square wave by summing two out of phase ramp waves. 
The duty cycle of the square wave is adjustable.

@param  uc_sample_index - This tells the function which sample to access on the time scale.
		uc_frequency - This tells the function which wavetable to access on the MIDI scale.
		uc_pulse_width - This determines the pulse_width. 0 = 50%, 127 = 1%

@return It returns an appropriate square wave sample value.
*/
unsigned char
calculate_square(unsigned char uc_sample_index, unsigned char uc_frequency, unsigned char uc_pulse_width)
{
	unsigned char uc_temp,
				  uc_sample,
				  uc_reverse_sample_index;
	signed int sn_temp;
	
	uc_reverse_sample_index = uc_sample_index - uc_pulse_width;
			
	uc_temp = pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_sample_index]);
		
	sn_temp = uc_temp - pgm_read_byte(&G_AUC_RAMP_WAVETABLE_LUT[uc_frequency][uc_reverse_sample_index]);

	/*Now we'll have a positive or negative number. We have to center it around 127 and make sure
	that the sample is never going to be over 255 or less than 0.*/
	if(sn_temp > 127)
	{
		uc_sample = 255;
	}
	else if(sn_temp < -128)
	{
		uc_sample = 0;
	}
	else
	{
		uc_sample = 128 + sn_temp;
	}
	
	return uc_sample;
}


/*
Linear Interpolation
The top 8 bits give us the index = x1
The bottom 8 bits give us the next index = x2
So, we add x1 + (x1-x2)*(bottom 7 bits)/128	
*/	
unsigned char 
linear_interpolate(unsigned char uc_reference, unsigned char uc_sample_1, unsigned char uc_sample_2)
{
	signed int sn_difference;
	unsigned char uc_result;
		
	sn_difference = uc_sample_2 - uc_sample_1;
	
	sn_difference *= uc_reference;
	
	sn_difference >>= 8;
	
	uc_result = uc_sample_1 + sn_difference;

	return uc_result;
	
}
