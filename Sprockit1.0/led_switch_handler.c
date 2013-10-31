/* 
@file led_switch_handler.c

@brief

This file contains functions related to the led and switch interactions.
Pressing switches updates LEDs and general synthesizer settings.
This file contains the functions needed to tell the i/o expander which LED
to light and what setting to change based on a switch press.

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
#include <led_switch_handler.h>
#include <io.h>
#include <lfo.h>


//Local Variable Definitions
static unsigned char uc_led_lfo_dest_state;
static unsigned char uc_led_lfo_shape_state;

/*
@brief This function handles the i/o expanders as in updating LEDs and handling button presses.
When a button is pressed, we have to determine which one and take the appropriate action.

@param This function takes no parameters and returns no vals.
*/
void
led_switch_handler(g_setting *p_global_setting, unsigned char uc_pressed_btn_index)
{
		
	/*Now that we know which button was pressed, 
	we can turn on the appropriate LED.
	There is a case statement for each tact switch.
	We also update the appropriate setting.*/
	switch(uc_pressed_btn_index)
	{
		case TACT_LFO_SHAPE:
			
			/*Increment the state of the led and loop back around if necessary*/
			if(uc_led_lfo_shape_state++ >= NUM_OF_LFO_SHAPES - 1)
			{				
					uc_led_lfo_shape_state = LFO_SHAPE_1;
			}
			
			p_global_setting->auc_synth_params[LFO_WAVESHAPE] = uc_led_lfo_shape_state;
			
			
			/*Set the LEDs appropriately*/
			set_lfo_shape_leds();
			
		break;
		
		case TACT_LFO_DEST:

			/*If the LFO was modifying the amplitude, then we need to set the 
			synth_params value to it's maximum so that it doesn't get stuck at a low value*/
			if(AMPLITUDE == auc_lfo_dest_decode[p_global_setting->auc_synth_params[LFO_DEST]])
			{
				p_global_setting->auc_synth_params[AMPLITUDE] = 255;
			}
			else if(PITCH_SHIFT == auc_lfo_dest_decode[p_global_setting->auc_synth_params[LFO_DEST]])
			{
				p_global_setting->auc_synth_params[PITCH_SHIFT] = 127;
			}

			/*Increment the state of the led and loop back around if necessary*/
			if(uc_led_lfo_dest_state++ >= NUM_OF_LFO_DESTINATIONS - 1)
			{				
					uc_led_lfo_dest_state = LFO_DEST_1;
			}

			p_global_setting->auc_synth_params[LFO_DEST] = uc_led_lfo_dest_state;
			
			set_lfo_dest_leds();
			
			
		break;
		
		
					
		default:
						
		break;

	}    
}

void
set_lfo_dest_led_state(unsigned char uc_new_state)
{
	uc_led_lfo_dest_state = uc_new_state;
}

void
set_lfo_dest_leds(void)
{
  	switch(uc_led_lfo_dest_state)
  	{		
	    case LFO_DEST_1:
		
			PORTD |= ~LED_DEST_3_MASK;	
			PORTC &= LED_DEST_1_MASK;

		break;

		case LFO_DEST_2:
		
			PORTC |= ~LED_DEST_1_MASK;	
			PORTB &= LED_DEST_2_MASK;

		break;

		case LFO_DEST_3:
		
			PORTB |= ~LED_DEST_2_MASK;	
			PORTD &= LED_DEST_3_MASK;

		break;
		
		default:

		break;
 	}

}

void
set_lfo_shape_led_state(unsigned char uc_new_state)
{
	uc_led_lfo_shape_state = uc_new_state;
}

void
set_lfo_shape_leds(void)
{
	switch(uc_led_lfo_shape_state)
  	{		
	    case LFO_SHAPE_1:
		
			PORTC |= ~LED_SHAPE_3_MASK;	
			PORTC &= LED_SHAPE_1_MASK;

		break;

		case LFO_SHAPE_2:
		
			PORTC |= ~LED_SHAPE_1_MASK;	
			PORTC &= LED_SHAPE_2_MASK;

		break;

		case LFO_SHAPE_3:
		
			PORTC |= ~LED_SHAPE_2_MASK;	
			PORTC &= LED_SHAPE_3_MASK;

		break;
		
		default:

		break;
 	}
}

void
led_init(void)
{	
	set_lfo_dest_led_state(LFO_DEST_1);
	set_lfo_shape_led_state(LFO_SHAPE_1);
	
	set_lfo_dest_leds();
	set_lfo_shape_leds();
	
}






