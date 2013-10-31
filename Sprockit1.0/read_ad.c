/* 
@file read_ad.c

@brief

This file contains the ad reading functions. We have to step through each knob and each of the two multiplexers.
We read them one by one and compare that to the old value to determine if we need to update a parameter. There
are plenty of variables about when to actually update a parameter. Things like loading patches and such cause
parameters to be read from different places and to be updated or not. Check it out!

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
#include <read_ad.h>
#include <lfo.h>
#include <midi.h>
#include <led_switch_handler.h>
#include <oscillator.h>

/*
@brief This function handles reading all the pots and updating the appropriate value in the appropriate place.

@param It takes the global setting structure as input

@return It doesn't return anything. It sets values in the global synth array function.
*/
void 
read_ad(volatile g_setting *p_global_setting)
{
	//local variables
	static unsigned char uc_ad_index = 0;//index to keep track of where we are in cycling through the AD channels and pots
	unsigned char uc_temp1 = 0;
	unsigned char uc_stored_value;

	/*
		-read the AD,
		-compare the result to the value in the array
		-if the values don't match, a knob was turned
		-change the value in the parameter array
		-start the next AD read
	*/

	//read the AD
	uc_temp1 = ADCH;

	/*We need to load the stored value to compare the new reading to.*/
	uc_stored_value = p_global_setting->auc_ad_values[uc_ad_index];
	
	//test the result, if it's not the same as the stored value,
	//update the value in the array and update the parameter array                                               
	if(((uc_stored_value < 254) && uc_temp1 > uc_stored_value + 1)
		|| ((uc_stored_value > 1) && uc_temp1 < uc_stored_value - 1))
	{
	
		/*Store the new value in the ad reading array*/
		p_global_setting->auc_ad_values[uc_ad_index] = uc_temp1;
		
		/*If the LFO is operating on a parameter, don't modify it here!
		Also, if the looping function is active, we don't want to change it here*/
		if((uc_ad_index != auc_lfo_dest_decode[p_global_setting->auc_synth_params[LFO_DEST]]))
		{
			p_global_setting->auc_synth_params[uc_ad_index] = uc_temp1;
		}
		
		if(uc_ad_index == OSC_WAVESHAPE)
		{
			decode_oscillator_waveshape(p_global_setting, uc_temp1);
		}
		else if(uc_ad_index == ADSR_LENGTH)
		{
			decode_adsr_length(p_global_setting, uc_temp1);
		}
		
		/*set the flag in the modified flag array to know that the pot value is what we want and not 
		the value loaded from the eeprom for a loaded patch
		or a value transmitted by MIDI*/
		p_global_setting->auc_parameter_source[uc_ad_index] = SOURCE_AD;
	}
	
	uc_ad_index++;	

	//if we've cycled through all the knobs
	if(uc_ad_index == NUMBER_OF_KNOBS)
	{
		uc_ad_index = 0;
	}
		
	//set the analog multiplexer
	//since the 8 pots are multiplexed down to two - divide the index by two
	uc_temp1 = uc_ad_index >> 1;
	set_pot_mux_sel(uc_temp1);

	//set the ADC input
	uc_temp1 = uc_ad_index%2;//cycle through AD 0 to 3 by modding the index (1 mod 4 = 1) (5 mod 4 = 1)
	ADMUX = 0x20 | uc_temp1;//the two here keeps the mux left adjusted, and we set which input we're reading
	
	//start the next conversion
	SET_BIT(ADCSRA, ADSC);
}

void
set_pot_mux_sel(unsigned char uc_index)
{
	
	switch (uc_index)
	{
		case 0:
			
			CLEAR_BIT(PORTD, PD6);
			CLEAR_BIT(PORTD, PD7);

		break;

		case 1:

			SET_BIT(PORTD, PD6);

		break;

		case 2:

			CLEAR_BIT(PORTD, PD6);
			SET_BIT(PORTD, PD7);

		break;

		case 3:

			SET_BIT(PORTD, PD6);

		break;

		default:
		break;
	}
}

void
initialize_pots(g_setting *p_global_setting)
{
	unsigned char 	uc_ad_index,
					uc_temp1,
					uc_ad_reading;

	for(uc_ad_index = 0; uc_ad_index < NUMBER_OF_KNOBS; uc_ad_index++)
	{
		//set the analog multiplexer
		//since the 8 pots are multiplexed down to two - divide the index by four
		uc_temp1 = uc_ad_index >> 1;
		set_pot_mux_sel(uc_temp1);

		//set the ADC input
		uc_temp1 = uc_ad_index%2;//cycle through AD 0 to 3 by modding the index (1 mod 4 = 1) (5 mod 4 = 1)
		ADMUX = 0x20 | uc_temp1;//the two here keeps the mux left adjusted, and we set which input we're reading
		
		//start the next conversion
		SET_BIT(ADCSRA, ADSC);

		while(CHECK_BIT(ADCSRA, ADSC));

		uc_ad_reading = ADCH;

		p_global_setting->auc_ad_values[uc_ad_index] = uc_ad_reading;
		p_global_setting->auc_synth_params[uc_ad_index] = uc_ad_reading;
		
		if(uc_ad_index == OSC_WAVESHAPE)
		{
			decode_oscillator_waveshape(p_global_setting, uc_temp1);
		}
		else if(uc_ad_index == ADSR_LENGTH)
		{
			decode_adsr_length(p_global_setting, uc_temp1);
		}
	}
}

