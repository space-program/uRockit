/*
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

#ifndef LED_SWITCH_HANDLER_H
#define LED_SWITCH_HANDLER_H

//The number of each tact switch in terms of the input on the i/o expander.
//The input number shows up as a zero in the byte.

/*The mask for each LED, basically a 1 at whichever number matches the LED input*/
#define LED_DEST_1_MASK			0xDF
#define LED_DEST_2_MASK			0xFE
#define LED_DEST_3_MASK			0xDF

#define LED_SHAPE_1_MASK		0xEF	
#define LED_SHAPE_2_MASK		0xF7
#define LED_SHAPE_3_MASK		0xFB

#define NUM_OF_LFO_DESTINATIONS 3
#define NUM_OF_LFO_SHAPES		3

//The states for the LFO Destination LEDs.
#define LFO_DEST_1   	0
#define LFO_DEST_2   	1
#define LFO_DEST_3   	2

//The states for the LFO Shape LEDs.
#define LFO_SHAPE_1		0
#define LFO_SHAPE_2		1
#define LFO_SHAPE_3		2

#define TACT_LFO_DEST	0
#define TACT_LFO_SHAPE	1

//Function prototypes
void
led_switch_handler(g_setting *p_global_setting, unsigned char uc_pressed_btn_index);

void
led_init(void);

void
set_lfo_dest_led_state(unsigned char uc_new_state);

void
set_lfo_dest_leds(void);

void
set_lfo_shape_led_state(unsigned char uc_new_state);

void
set_lfo_shape_leds(void);

void
led_mux_handler(void);

#endif /*LED_SWITCH_HANDLER_H*/
