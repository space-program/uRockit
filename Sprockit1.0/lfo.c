/* 
@file lfo.c

@brief

This contains the function for the LFO which modifies a parameter using an oscillator waveform.
The LFO modifies one of the values using the oscillator.
The rate is set by the LFO_RATE parameter.
The amount of modification is set by the LFO_AMOUNT parameter.
The LFO takes its value to modify from one of two sources depending on whether or not the target value has
been modified by the user - whether or not the value has been modified is stored.
If the value has not been modified after loading a new patch, the value is taken from the loaded patch array
If the value has been modified, which we know by an AD value being updated, the value is taken from the AD reading

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
#include <lfo.h>
#include <wavetables.h>
#include <midi.h>


/*This array is a decoder for which synth parameter is being effected by the
LFO. To get it to access different parameters, make changes here.*/
const unsigned char auc_lfo_dest_decode[8] =	{AMPLITUDE,
										   		FILTER_FREQUENCY,
												PITCH_SHIFT,
										   		FILTER_Q,
										   		OSC_MIX,
												OSC_DETUNE,
												FILTER_ENV_AMT,
												FILTER_ATTACK};

/*Want faster or slower, muck with this*/
const unsigned int g_aun_lfo_rate_lut[32]= 	{1,2,4,8,16,32,48,64,80,96,112,128,192,
											224,256,288,320,352,384,448,
											512,576,640,704,778,896,1024,
											1280,1536,2048,2560,3072};

void lfo(g_setting *p_global_setting)
{	
	static unsigned int un_lfo_reference;
	
	signed int 		sn_temp1;

	unsigned char 	uc_temp1,
					uc_lfo_dest,
					uc_lfo_initial_param = 0,
					uc_lfo_amount,
					uc_modifier = 0,
					uc_wave_shape,
					uc_parameter_source,
					lfsr_bit,
					uc_reverse_index;

	unsigned int 	un_lfo_rate,
					un_modifier_calc;
	
	static unsigned int lfsr = 0xACE1; 
						
	static unsigned char uc_morph_index,
						 uc_morph_timer,
						 uc_morph_state;
	
	
	/*Sync the lfos by resetting the reference if the lfo sync parameter is set and the 
		flag denoting a note on is set*/
	if(p_global_setting->auc_synth_params[LFO_SYNC] == 1 &&
	   g_uc_lfo_midi_sync_flag == 1)
	{
		un_lfo_reference = 0;
		g_uc_lfo_midi_sync_flag = 0;
		uc_morph_timer = 0;
		uc_morph_index = 0;
		uc_morph_state = 0;

	}
		
		uc_lfo_amount = p_global_setting->auc_synth_params[LFO_AMOUNT];//how much the parameter will vary
		uc_temp1 = p_global_setting->auc_synth_params[LFO_RATE]>>3;//the rate is in a lookup table because the 
		un_lfo_rate = g_aun_lfo_rate_lut[uc_temp1];
		uc_lfo_dest = auc_lfo_dest_decode[p_global_setting->auc_synth_params[LFO_DEST]];//values 1-255 aren't sufficient
		uc_wave_shape = p_global_setting->auc_synth_params[LFO_WAVESHAPE];
		
		uc_parameter_source = p_global_setting->auc_parameter_source[uc_lfo_dest];

		switch(uc_parameter_source)
		{
			case SOURCE_AD:

				uc_lfo_initial_param = p_global_setting->auc_ad_values[uc_lfo_dest];

			break;

			case SOURCE_EXTERNAL:

				uc_lfo_initial_param = p_global_setting->auc_external_params[uc_lfo_dest];

			break;

			default:

				uc_lfo_initial_param = 0;
				
			break;
		}
	
		sn_temp1 = uc_lfo_initial_param;			

		if(uc_lfo_amount > 1)
		{
			
			//get the value to modify, determined by looking at which value and whether that bit has been set
			//in the unModifiedValueFlag bit vector
			//the unModifiedValueFlag is set by the readAD algorithm, a 1 means the value has been changed
			//from the loaded value, 0 means that we use the value loaded from eeprom
			//the bit we look at is the destination of the LFO
	
			//now that we have the value, modify it using the oscillator algorithm
			//we update the sample reference using two 
			switch(uc_wave_shape)
			{
				case SQUARE:
				
					uc_temp1 = un_lfo_reference >> 7;
					uc_modifier = G_AUC_SQUARE_SIMPLE_WAVETABLE_LUT[uc_temp1];
				
				break;
						
				case RAMP:
			
					uc_temp1 = un_lfo_reference >> 7;
					uc_modifier = G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_temp1];
			
				break;
					
				case TRIANGLE:
				
					uc_temp1 = un_lfo_reference >> 7;
					uc_modifier = G_AUC_TRIANGLE_SIMPLE_WAVETABLE_LUT[uc_temp1];
				
				break;
				
				case SIN:
				
					uc_temp1 = un_lfo_reference >> 7;			
					uc_modifier = G_AUC_SIN_LUT[uc_temp1];
				
				break;

				case MORPH_1:
				
					if(uc_morph_timer == 0)
					{
						uc_morph_index++;
						uc_morph_timer = p_global_setting->auc_synth_params[LFO_RATE] >> 3;
					}
					
					uc_morph_timer--;
					
					uc_temp1 = un_lfo_reference >> 7;
					
					un_modifier_calc = G_AUC_SIN_LUT[uc_temp1];
					un_modifier_calc = un_modifier_calc * G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_morph_index];
					un_modifier_calc = un_modifier_calc >> 8;
					
					uc_modifier = (unsigned char) un_modifier_calc;
					
				
				break;
				
				case MORPH_2:
				
					if(uc_morph_timer == 0)
					{
						uc_morph_index++;
						uc_morph_timer = p_global_setting->auc_synth_params[LFO_RATE] >> 3;
					}
					
					uc_morph_timer--;
					
					uc_temp1 = un_lfo_reference >> 7;
					
					un_modifier_calc = G_AUC_TRIANGLE_SIMPLE_WAVETABLE_LUT[uc_temp1];

					uc_temp1 = uc_morph_index >> 1;
					un_modifier_calc = un_modifier_calc * G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_temp1];
					un_modifier_calc = un_modifier_calc >> 8;
					
					uc_modifier = (unsigned char) un_modifier_calc;
				
				break;
				
				case MORPH_3:
				
					if(uc_morph_timer == 0)
					{
						uc_morph_index++;
						uc_morph_timer = p_global_setting->auc_synth_params[LFO_RATE] >> 3;
					}
					
					uc_morph_timer--;
					
					uc_temp1 = un_lfo_reference >> 7;
			
					uc_reverse_index = uc_temp1- uc_morph_index;
			
					uc_temp1 = G_AUC_TRIANGLE_SIMPLE_WAVETABLE_LUT[uc_temp1];
		
					sn_temp1 = uc_temp1 - G_AUC_SQUARE_SIMPLE_WAVETABLE_LUT[uc_reverse_index];

					/*Now we'll have a positive or negative number. We have to center it around 127 and make sure
					that the sample is never going to be over 255 or less than 0.*/
					if(sn_temp1 > 127)
					{
						uc_modifier = 255;
					}
					else if(sn_temp1 < -128)
					{
						uc_modifier = 0;
					}
					else
					{
						uc_modifier = 128 + sn_temp1;
					}
				
				break;
				
				case MORPH_4:
				
					if(uc_morph_timer == 0)
					{
						uc_morph_index++;
						uc_morph_timer = p_global_setting->auc_synth_params[LFO_RATE] >> 3;
					}
					
					uc_morph_timer--;
					
					uc_temp1 = un_lfo_reference >> 7;
					
					un_modifier_calc = G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_temp1];
					un_modifier_calc = un_modifier_calc * G_AUC_TRIANGLE_SIMPLE_WAVETABLE_LUT[uc_morph_index];
					un_modifier_calc = un_modifier_calc >> 8;
					
					uc_modifier = (unsigned char) un_modifier_calc;
				
				break;
				
				case MORPH_5:
				
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

						uc_morph_timer = p_global_setting->auc_synth_params[LFO_RATE] >> 3;
					}
					
					uc_morph_timer--;
					
					uc_temp1 = un_lfo_reference >> 7;
			
					uc_reverse_index = uc_temp1- uc_morph_index;
			
					uc_temp1 = G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_temp1];
		
					sn_temp1 = uc_temp1 - G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_reverse_index];

					/*Now we'll have a positive or negative number. We have to center it around 127 and make sure
					that the sample is never going to be over 255 or less than 0.*/
					if(sn_temp1 > 127)
					{
						uc_modifier = 255;
					}
					else if(sn_temp1 < -128)
					{
						uc_modifier = 0;
					}
					else
					{
						uc_modifier = 128 + sn_temp1;
					}
					
				
				break;
				
				case MORPH_6:

					if(uc_morph_timer == 0)
					{
						uc_morph_index++;
						uc_morph_timer = p_global_setting->auc_synth_params[LFO_RATE] >> 3;
					}
					
					uc_morph_timer--;
					
					uc_temp1 = un_lfo_reference >> 7;
					
					un_modifier_calc = G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_temp1];
					un_modifier_calc = un_modifier_calc * G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_morph_index];
					un_modifier_calc = un_modifier_calc >> 8;
					
					uc_modifier = (unsigned char) un_modifier_calc;
				
				break;
				
				case MORPH_7://reverse ramp presently

					uc_temp1 = un_lfo_reference >> 7;
					uc_temp1 = 255 - uc_temp1;
					uc_modifier = G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_temp1];

				break;
				
				case MORPH_8:
				
					/*This morphing waveshape is a square wave with varying pulse width.*/
		
					if(uc_morph_timer == 0)
					{
						uc_morph_index++;
						uc_morph_timer = p_global_setting->auc_synth_params[LFO_RATE] >> 3;
					}
					
					uc_morph_timer--;
					
					uc_temp1 = un_lfo_reference >> 7;
			
					uc_reverse_index = uc_temp1- uc_morph_index;
			
					uc_temp1 = G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_temp1];
		
					sn_temp1 = uc_temp1 - G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_reverse_index];

					/*Now we'll have a positive or negative number. We have to center it around 127 and make sure
					that the sample is never going to be over 255 or less than 0.*/
					if(sn_temp1 > 127)
					{
						uc_modifier = 255;
					}
					else if(sn_temp1 < -128)
					{
						uc_modifier = 0;
					}
					else
					{
						uc_modifier = 128 + sn_temp1;
					}
				
					
				break;
				
				case MORPH_9:

					uc_temp1 = un_lfo_reference >> 8;			
					uc_modifier = G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_temp1];

				break;

				case HARD_SYNC:

					uc_temp1 = un_lfo_reference >> 8;			
					uc_modifier = G_AUC_RAMP_SIMPLE_WAVETABLE_LUT[uc_temp1];

				break;
				
				case NOISE:
				
					/*A pseudo random number is generated using a linear feedback shift register
					The polynomial expression used is: x^16 + x^14 + x^13 + x^11 + 1*/

					lfsr_bit = ((lfsr >> 15) ^ (lfsr >> 13) ^ (lfsr ^ 12) ^ (lfsr >> 10)) & 1;
					lfsr = (lfsr << 1) | (lfsr_bit); 
        
					uc_modifier = (unsigned char) lfsr;
				
				break;
				
				case RAW_SQUARE:
				
					uc_temp1 = un_lfo_reference >> 8;
					
					if(uc_temp1 > 127)
					{
						uc_modifier = 0;
					}
					else
					{
						uc_modifier = 255;
					}
				
				break;

				default:
					//Default to sin
					uc_temp1 = un_lfo_reference >> 7;			
					uc_modifier = G_AUC_SIN_LUT[uc_temp1];

				break;

			}


			//modify that value based on the current lfo Reference value and the LFO amount parameter
			//multiply and divide scaling to get the value
			sn_temp1 = uc_modifier;//max value 255
			sn_temp1 -= 128;//from -128 to 127
			sn_temp1 *= uc_lfo_amount;//-32640 to 32385 
			sn_temp1 = sn_temp1 >> 7;

			//store the modified value in the Synth Params array

			sn_temp1 += uc_lfo_initial_param;

			if(sn_temp1 > 255)
			{
				sn_temp1 = 255;
			}
			else if(sn_temp1 < 0)
			{
				sn_temp1 = 0;
			}
		
			p_global_setting->auc_synth_params[uc_lfo_dest] = sn_temp1;
		
		}
		else/*If the LFO amount is zero, the we should pass the ad value directly through to the 
			synth params array*/
		{
			if(uc_lfo_dest != AMPLITUDE)
			{
				p_global_setting->auc_synth_params[uc_lfo_dest] = uc_lfo_initial_param;
			}
			else
			{
				p_global_setting->auc_synth_params[uc_lfo_dest] = 255;
			}
		}
		
		
			
			//if(un_lfo_reference > 24000)
			//{
				//set_lfo_dest_led_state(2);
				//set_lfo_dest_leds();
			//}
			//else if(un_lfo_reference > 12000)
			//{
				//set_lfo_dest_led_state(1);
				//set_lfo_dest_leds();
			//}
			//else
			//{
				//set_lfo_dest_led_state(0);
				//set_lfo_dest_leds();
			//}
		

		/*The LFO is ordinarily free running but will sychronize by zeroing out
		the LFO reference if the midi sync flag is set.
		increment the reference for the oscillator*/

		un_lfo_reference += un_lfo_rate;

		if(un_lfo_reference >= SAMPLE_MAX)
		{
			un_lfo_reference -= SAMPLE_MAX;
		}

}


