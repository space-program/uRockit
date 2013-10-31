/*
@file filter.c

@brief This module handles the filter frequency and filter q, or resonance. It calculates what it needs to be
and transmits that value over the SPI bus to the digital pots.


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
#include <filter.h>
#include <io.h>
#include <spi.h>
#include <led_switch_handler.h>

void
inline frequency_cs_enable(void);

void
inline resonance_cs_enable(void);

static int
antilog(unsigned char uc_linear);

/*
@brief filter calculates what the filter frequency should be and what the filter q should be. It handles
the filter envelope to get the right value. Then, it transmits that value to the digital pots to actually
set these parameters for the analog filter. It also turns on the right op amps to change between
low,band, and high-pass filters.

@param It takes the global setting array.

@return It returns nothing.
*/
void 
filter(g_setting *p_global_setting)
{

	static signed long sl_adsr_temp_adder;//This variable stores the adder for the adsr calculations.
	
	static signed int	sn_adsr_adder_increment,//The amount we are incrementing the temp adder
						sn_target_envelope_value,//This is used for calculating
						sn_filter_freq_calc_temp;

	
	static unsigned int un_antilog_filter_frequency,//This stores the value of the filter after using the antilog LUT
						
						un_last_written_filter_value;//This stores the last value that was written to the filter.
						
						
	static unsigned char	uc_filter_1_value, //The value for filter pot 1
							uc_filter_2_value,	//The value for filter pot 2
							uc_antilog_q_value,
							uc_update_state,
							uc_adsr_state,//keep track of filter envelope
							uc_adsr_timer,//Remember the Q from the last time around
							uc_adsr_step_count;//Where we are in the envelope
						 
	unsigned char	uc_sustain_level, //The level that the sustain will hold. How we know when we get there.
					uc_command,//The command sent to the digital potentiometer
					uc_filter_frequency,
					uc_filter_q,
					uc_filter_envelope_amount,
					uc_remaining_number_of_adsr_steps,
					uc_filter_type;//The type of filter: high pass, low pass, band pass
	
	//if the key is released, move directly to release
	if(0 == g_uc_key_press_flag)
	{
		/*If we are droning or looping, we want the envelope to loop*/
		if(g_uc_drone_flag == FALSE)
		{
			sn_adsr_adder_increment = sl_adsr_temp_adder/uc_adsr_step_count;
			uc_adsr_state = RELEASE_STATE;
		}		
	}

	/*Get the filter frequency*/
	uc_filter_frequency = p_global_setting->auc_synth_params[FILTER_FREQUENCY];

	/*Get the filter q*/
	uc_filter_q = p_global_setting->auc_synth_params[FILTER_Q];

	uc_filter_envelope_amount = p_global_setting->auc_synth_params[FILTER_ENV_AMT];
	
	uc_sustain_level = p_global_setting->auc_synth_params[FILTER_SUSTAIN] >> 1;
	
	/*Calculate the filter envelope*/
	/*This is how it works-
	Example: The filter envelope pot is set to 255, we convert this to an envelope setting of +127.
	There are 128 steps and we need to get to filter frequency + 127.
	So, we add 128/128 128 times and we get +127 
	The range of the filter envelope pot is -128 to +127, so we subtract 128.
	Then we multiply by two to make the range -256 to +254*/

	
	
	/*If a key is pressed, start the envelope at the beginning*/
	if(1 == g_uc_filter_envelope_sync_flag)
	{	
		uc_adsr_state = ATTACK_STATE;
		uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_ATTACK]>>2;
		
		sn_target_envelope_value = uc_filter_envelope_amount - 128;
		sn_target_envelope_value <<= 8;
		sn_target_envelope_value = sn_target_envelope_value - sl_adsr_temp_adder;
		
				
		uc_remaining_number_of_adsr_steps = NUMBER_OF_FILTER_ADSR_STEPS-uc_adsr_step_count;
		
		if(uc_remaining_number_of_adsr_steps > 0)
		{
			sn_adsr_adder_increment = sn_target_envelope_value/uc_remaining_number_of_adsr_steps;
		}
		else
		{
			sn_adsr_adder_increment = 0;	
		}						
			
		g_uc_filter_envelope_sync_flag = 0;
	}
	
	/*The adder holds the addition of the factor 256 times*/	
	sn_filter_freq_calc_temp = sl_adsr_temp_adder >> LOG_NUMBER_OF_FILTER_ADSR_STEPS;
	sn_filter_freq_calc_temp += uc_filter_frequency;
	
	//set_led_display(uc_filter_frequency >> 4);//DIAGNOSTIC

	/*We limit the range to the min and the max. If we don't, bad things happen.*/
	if(sn_filter_freq_calc_temp > 255)
	{
		uc_filter_frequency = 255;
	}
	else if(sn_filter_freq_calc_temp < 0)
	{
		uc_filter_frequency = 0;
	}
	else
	{
		uc_filter_frequency = sn_filter_freq_calc_temp;
	}
	
	/*This filter ADSR envelope is very similar to the amplitude ADSR envelope. It functions the same way. 
	It has a central timer. This timer is allowed to decrement down to zero. The number of times that 
	it is allowed to decrement down to zero determines how long the filter will remain in each state.
	*/
	if(uc_adsr_timer > 0)
	{
		uc_adsr_timer--;		
	}
	else
	{
		switch(uc_adsr_state)
		{
			case ATTACK_STATE:
					
					/*Check if it's time to start Decay*/
					if(uc_adsr_step_count == MAX_FILTER_ADSR_STEP)
					{
						sn_adsr_adder_increment = sl_adsr_temp_adder >> 7;
						uc_adsr_state = DECAY_STATE;
						uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_DECAY]>>3;
					}
					
					if(p_global_setting->auc_synth_params[FILTER_ATTACK] < 32)
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_ATTACK]>>4;
						
						if(uc_adsr_step_count < MAX_FILTER_ADSR_STEP - 8)
						{
							uc_adsr_step_count += 8;
							sl_adsr_temp_adder += 8*sn_adsr_adder_increment;
						}
						else
						{
							sl_adsr_temp_adder += (MAX_FILTER_ADSR_STEP - uc_adsr_step_count)*sn_adsr_adder_increment;		
							uc_adsr_step_count = MAX_FILTER_ADSR_STEP;
						}
					}
					else if(p_global_setting->auc_synth_params[FILTER_ATTACK] < 128)
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_ATTACK]>>3;
						
						if(uc_adsr_step_count < MAX_FILTER_ADSR_STEP - 4)
						{
							uc_adsr_step_count += 4;
							sl_adsr_temp_adder += 4*sn_adsr_adder_increment;
						}
						else
						{
							sl_adsr_temp_adder += (MAX_FILTER_ADSR_STEP - uc_adsr_step_count)*sn_adsr_adder_increment;	
							uc_adsr_step_count = MAX_FILTER_ADSR_STEP;
						}
					}
					else if(p_global_setting->auc_synth_params[FILTER_ATTACK] < 192)
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_ATTACK]>>2;
						
						if(uc_adsr_step_count < MAX_FILTER_ADSR_STEP - 2)
						{
							uc_adsr_step_count += 2;
							sl_adsr_temp_adder += 2*sn_adsr_adder_increment;
						}
						else
						{
							sl_adsr_temp_adder += (MAX_FILTER_ADSR_STEP - uc_adsr_step_count)*sn_adsr_adder_increment;	
							uc_adsr_step_count = MAX_FILTER_ADSR_STEP;
						}
					}
					else
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_ATTACK]>>1;
						
						if(uc_adsr_step_count < 253)
						{
							uc_adsr_step_count++;
							sl_adsr_temp_adder += sn_adsr_adder_increment;
						}
						else
						{
							sl_adsr_temp_adder += (MAX_FILTER_ADSR_STEP - uc_adsr_step_count)*sn_adsr_adder_increment;	
							uc_adsr_step_count = MAX_FILTER_ADSR_STEP;
						}
					}
			break;

			case DECAY_STATE:


				if(p_global_setting->auc_synth_params[FILTER_DECAY] < 32)
				{
						uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_DECAY]>>3;
						
						if(uc_adsr_step_count > uc_sustain_level + 6)
						{
							uc_adsr_step_count -= 6;
							sl_adsr_temp_adder -= 6*sn_adsr_adder_increment;
						}
						else
						{
							sl_adsr_temp_adder -= (uc_adsr_step_count - uc_sustain_level)*sn_adsr_adder_increment;	
							uc_adsr_step_count = uc_sustain_level;
						}
				}
				else if(p_global_setting->auc_synth_params[FILTER_DECAY] < 128)
				{
					uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_DECAY]>>3;
						
					if(uc_adsr_step_count > uc_sustain_level + 4)
					{
						uc_adsr_step_count -= 4;
						sl_adsr_temp_adder -= 4*sn_adsr_adder_increment;
					}
					else
					{
						sl_adsr_temp_adder -= (uc_adsr_step_count - uc_sustain_level)*sn_adsr_adder_increment;	
						uc_adsr_step_count = uc_sustain_level;
					}
				}
				
				else if(p_global_setting->auc_synth_params[FILTER_DECAY] < 192)
				{
					uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_DECAY]>>2;
						
					if(uc_adsr_step_count > uc_sustain_level + 2)
					{
						uc_adsr_step_count -= 2;
						sl_adsr_temp_adder -= 2*sn_adsr_adder_increment;
					}
					else
					{
						sl_adsr_temp_adder -= (uc_adsr_step_count - uc_sustain_level)*sn_adsr_adder_increment;	
						uc_adsr_step_count = uc_sustain_level;
					}
				}
				else
				{
					uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_DECAY]>>1;
					uc_adsr_step_count--;
					sl_adsr_temp_adder -= sn_adsr_adder_increment;
						
				}

				if(uc_adsr_step_count == uc_sustain_level)
				{
					uc_adsr_state = SUSTAIN_STATE;
				}	

			break;

			case SUSTAIN_STATE:
			
				/*If we are droning or looping, we don't stop for sustain*/
				if(g_uc_drone_flag == TRUE)
				{
					uc_adsr_state = RELEASE_STATE;
				}

			break;

			case RELEASE_STATE:
			
				if(uc_adsr_step_count > 0)
				{
					if(p_global_setting->auc_synth_params[FILTER_RELEASE] < 16)
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_RELEASE]>>3;
						
						if(uc_adsr_step_count > 4)
						{
							uc_adsr_step_count -= 4;
							sl_adsr_temp_adder -= 4*sn_adsr_adder_increment;
						}
						else
						{
							sl_adsr_temp_adder = 0;	
							uc_adsr_step_count = 0;
						}
					}
				
					else if(p_global_setting->auc_synth_params[FILTER_RELEASE] < 64)
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_RELEASE]>>2;
						
						if(uc_adsr_step_count > 2)
						{
							uc_adsr_step_count -= 2;
							sl_adsr_temp_adder -= 2*sn_adsr_adder_increment;
						}
						else
						{
							sl_adsr_temp_adder = 0;	
							uc_adsr_step_count = 0;
						}
					}
					else
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_DECAY]>>1;
						uc_adsr_step_count--;
						sl_adsr_temp_adder -= sn_adsr_adder_increment;
						
					}
				}
				else
				{
					uc_adsr_state = ATTACK_STATE;
					uc_adsr_timer = p_global_setting->auc_synth_params[FILTER_ATTACK]>>5;
					uc_adsr_step_count = 0;
					sl_adsr_temp_adder = 0;
					
					if(g_uc_drone_flag == TRUE)
					{
						g_uc_filter_envelope_sync_flag = 1;
					}
				}
				
			break;
			
			default:

			break;

		
			}
	}
	
	/*This state machine sets the frequency and resonance pots.*/
	
	/*We try to minimize sending things when we don't need to and when send the filter values, 
	it takes two turns because there are two pots and we are doing this antilog thing to make the 
	filter more linear.*/
	
	un_antilog_filter_frequency = antilog(uc_filter_frequency);
	
	
	/*If the frequency didn't change, there's no need to write it again. Same goes for the resonance.*/
	/*We write the frequency in two steps though.*/
	/*We have to make sure that we get around to writing the resonance, but it's secondary*/
	if((uc_update_state == FREQUENCY_2_UPDATE || uc_update_state == WAIT) && uc_antilog_q_value != (antilog(uc_filter_q)>>1))
	{
		uc_update_state = FILTER_Q_UPDATE;
		uc_antilog_q_value = antilog(uc_filter_q) >> 1;//only 256 levels for q
	}
	else if(uc_update_state == FREQUENCY_1_UPDATE)//Gotta update both pots
	{
		uc_update_state = FREQUENCY_2_UPDATE;
		
	}
	else if(un_last_written_filter_value != un_antilog_filter_frequency)
	{
		uc_update_state = FREQUENCY_1_UPDATE;
		
		/*We looked up the antilog value of the filter frequency.
		Now we divide it by two and make each filter half of that value.
		Then, if it's odd, we add 1 to the second filter value*/
		uc_filter_1_value = un_antilog_filter_frequency >> 1;
		uc_filter_2_value = uc_filter_1_value;
		
		if(uc_filter_2_value != 255 && un_antilog_filter_frequency%2 == 1)
		{
			uc_filter_2_value++;
		}			
		
	}
	else
	{
		uc_update_state = WAIT;
	}	

	/*This state machine alternates between updating the frequency pots or the filter q pots*/
	switch(uc_update_state)
	{

		case(FREQUENCY_1_UPDATE):
			
			//Set the port and pin so the interrupt pin knows which pin to release when it's finished
			//transmitting.
			uc_command = 0x12;//The command for write to pot 1.
            
            //Enable the chip select pin on the digital pot.
			frequency_cs_enable();

			//Send the SPI message.
			send_spi_two_bytes(uc_command, uc_filter_1_value);	
						

		break;

		case(FREQUENCY_2_UPDATE):
			
			//Set the port and pin so the interrupt pin knows which pin to release when it's finished
			//transmitting.
			uc_command = 0x11;//The command for write to pot 0.
            
            //Enable the chip select pin on the digital pot.
			frequency_cs_enable();

			//Send the SPI message.
			send_spi_two_bytes(uc_command, uc_filter_2_value);			

		break;

		case(FILTER_Q_UPDATE):
		
			uc_command = 0x13;//write to the Q pots

			//Enable the chip select pin on the digital pot.
			resonance_cs_enable();

			//Send the SPI message.
			send_spi_two_bytes(uc_command, uc_antilog_q_value);

		break;
		
		case WAIT:
			//do nothing
		break;	

		default:
		
		break;		
	}
}

/*This performs an antilog type conversion on a linear number. It's really a piecewise
multiplication. I don't have enough room for another table.*/
static int
antilog(unsigned char uc_linear)
{
	unsigned char uc_index;
	
	uc_index = uc_linear >> 5;//Divide down to 8 segments

	switch (uc_index)
	{
		case 0://0-31 -> 0-124
			return uc_linear * 4;
		break;
		case 1://32-63 -> 128-252
			return uc_linear*4;
		break;
		case 2://64-95 -> 254-316 
			uc_linear = uc_linear - 63;
			return 252 + uc_linear*2;
		break;
		case 3://96-127 ->316-380
			uc_linear = uc_linear - 95;
			return 316 + uc_linear*2;
		break;
		case 4://128-159 ->380-412
			uc_linear = 255 - uc_linear;
			return 511 - uc_linear;
		break;
		case 5://160-191 ->413-444
			/*We want the highest region to be linear 9 bit*/
			uc_linear = 255 - uc_linear;
			return 511 - uc_linear;
		break;
		case 6://191-222 ->445-476
			/*We want the highest region to be linear 9 bit*/
			uc_linear = 255 - uc_linear;
			return 511 - uc_linear;
		break;
		case 7://223-255 ->479-511
			/*We want the highest region to be linear 9 bit*/
			uc_linear = 255 - uc_linear;
			return 511 - uc_linear;
		break;
		default:
		
			return 511;
			
		break;	
	}
		

}


//inline functions
void 
inline frequency_cs_enable(void)
{
	PORTD &= ~0x02;
}

void 
inline resonance_cs_enable(void)
{
	PORTD &= ~0x10;
}

