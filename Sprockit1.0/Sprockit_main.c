/*
@file sprockit_main.c

@brief This module contains the main routine for the synth.  It handles
when to run each routine.  It manages all the subroutines, when they are
run and the priority for each. It looks simple but the subroutines and their
interactions are plenty complicated.


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

/*TO DO:
More MIDI accessible LFO Destinations and waveshapes
Fix the oscillator waveshapes
*/


#include <pgmspace.h>
#include <io.h>
#include <interrupt.h>
#include <sprockit_main.h>
#include <sys_init.h>
#include <spi.h>
#include <interrupt_routines.h>
#include <wavetables.h>
#include <oscillator.h>
#include <amp_adsr.h>
#include <filter.h>
#include <read_ad.h>
#include <led_switch_handler.h>
#include <lfo.h>
#include <midi.h>
#include <uart.h>
#include <calculate_pitch.h>


/*This global structure holds all the synth parameters. It is accessible to all portions of the code.
The pointer is what is used to pass the location of the structure to all functions*/
g_setting global_setting, *p_global_setting;

//Global Flags
/*Some people will say that these global flags are a bad idea and they are probably right. But, I started this way and I 
didn't want to go through the pain of eliminating them and risk screwing up the code right before I launched. 
Forgiveness, please!
*/
volatile unsigned char g_uc_sample_request_flag = 1;//flag used to set the output and request a new sample from the state machine
volatile unsigned char g_uc_slow_interrupt_flag = FALSE;//flag set to tell events to update their values
volatile unsigned char g_uc_key_press_flag = FALSE;//flag for when a key is pressed
volatile unsigned char g_uc_note_on_flag = FALSE;//flag for generating audio output
volatile unsigned char g_uc_ad_ready_flag = FALSE;//flag used to indicate when the AD has completed a reading
volatile unsigned char g_uc_spi_ready_flag = TRUE;//flag used to indicate that the SPI has completed transmission
volatile unsigned char g_uc_ext_int_flag_0 = 0;//This flag indicates an external interrupt has occurred on int0 pin.
volatile unsigned char g_uc_display_knob_position_flag;//This flag indicates that a knob value needs to be displayed
volatile unsigned char g_uc_lfo_midi_sync_flag;//This flag is used to sync events to the arrival of new note on messages.
volatile unsigned char g_uc_filter_envelope_sync_flag;//Syncs the filter envelope to notes being played
volatile unsigned char g_uc_drone_loop_flag = 0;//Flag indicates whether or not the drone/loop feature has been activated
volatile unsigned char g_uc_adsr_midi_sync_flag = 0;
volatile unsigned char g_uc_drone_flag;


/*This routine is the HNIC. It determines when all the subroutines run.
Messing with business in here can have lethal consequences for many things, particularly 
related to timing.	What needs to run when is one of the greatest challenges of this design.
*/
int main(void)
{
	
	
	MIDI_MESSAGE mm_incoming_message;
	MIDI_MESSAGE *p_mm_incoming_message = &mm_incoming_message;

	p_global_setting = &global_setting;//assign the pointer to point at the global_setting structure

	static unsigned char uc_aux_task_state;//keep track of auxilliary task state - filter, lfo, envelope


	cli();//disable interrupts
  	sys_init();
	sei();//enable interrupts

	initialize_pots(p_global_setting);

	/*Initialize global_settings.
	This routine sets some important initial values. Without them, some of the subroutines
	can be confused because they are expecting certain 0 points that indicate some function is
	not active.*/
	global_setting.auc_synth_params[ADSR_SUSTAIN] = 92;
	global_setting.auc_synth_params[ADSR_LENGTH] = 127;
	decode_adsr_length(p_global_setting, 127);
	global_setting.auc_synth_params[ADSR_DECAY] = 127;
	global_setting.auc_synth_params[ADSR_RELEASE] = 127;
	global_setting.auc_ad_values[PITCH_SHIFT] = 127;
	global_setting.auc_synth_params[PITCH_SHIFT] = 127;
	global_setting.auc_parameter_source[PITCH_SHIFT] = SOURCE_AD;
	global_setting.auc_ad_values[AMPLITUDE] = 192;
	global_setting.auc_synth_params[AMPLITUDE] = 255;//set amplitude
	global_setting.auc_parameter_source[AMPLITUDE] = SOURCE_AD;
	global_setting.uc_adsr_multiplier = ADSR_MIN_VALUE;//Initialize the ADSR to its minimum value
	global_setting.auc_synth_params[PORTAMENTO] = 0;
	global_setting.auc_synth_params[FILTER_ENV_AMT] = 128;
	global_setting.auc_synth_params[OSC_MIX] = 127;
	global_setting.auc_synth_params[OSC_2_WAVESHAPE] = SQUARE;

  for (; ;)
  { 
  	RESET_WATCHDOG;

	/*Has the slow interrupt occurred?*/
	if(1 == g_uc_slow_interrupt_flag)
	{

		//Calculate the adsr envelope value
		adsr(p_global_setting);

		//Set the amplitude of the Voltage-Controlled Amplifier
		set_amplitude(p_global_setting);
		
		if(g_un_switch_debounce_timer > 0)
		{
			g_un_switch_debounce_timer--;
		}
		else if(g_uc_ext_int_0_flag ==  TRUE)
		{
			led_switch_handler(p_global_setting, TACT_LFO_SHAPE);
			g_uc_ext_int_0_flag = FALSE;
			CLEAR_EXT_INTERRUPTS;
			ENABLE_EXT_INT_0;
		}
		else if(g_uc_ext_int_1_flag == TRUE)
		{
			led_switch_handler(p_global_setting, TACT_LFO_DEST);
			g_uc_ext_int_1_flag = FALSE;
			CLEAR_EXT_INTERRUPTS
			ENABLE_EXT_INT_1;
		}

		//Let's handle midi messages.
		//We have to check the receive uart.
		//If there is data in the uart receive buffer, check to see
		//if the incoming fifo has anything in it.  If it doesn't have midi messages
		//waiting to be handled, send it to the incoming midi handler routine. 
		//If there is stuff in the incoming midi fifo, put the new message next in the fifo.
		if(uart_rx_buffer_has_byte())
		{
			handle_incoming_midi_byte(uart_get_byte());
		}

		/*auxilliary tasks
		These tasks are handled one at a time, each time through the slow interrupt routine
		We have to do them one at a time because we can't do them all every time through the loop
		because we don't have enough clock cycles, nor do we really need to do them that way
		They are contained in a state machine
		The number of things to do here affects the timing of them.  Adding more things to do will affect timing
		Just think about it a minute before you do anything crazy like that, but if you must and if you want the 
		envelopes and LFOs to work at specific frequency then 
		you'll have to go through each of the things below
		*/
		switch(uc_aux_task_state)
		{
			case AUX_TASK_SPI:
			/*The SPI is shared by the i/o expanders and the digital pots of the 
			filter. These tasks are mutually exclusive though, only one at a time.*/

			    spi();

				uc_aux_task_state = AUX_TASK_READ_AD;				

			break;

			case AUX_TASK_READ_AD:

				if(!CHECK_BIT(ADCSRA, ADSC))
				{
						read_ad(p_global_setting);
						g_uc_ad_ready_flag = 0;
				}
				
				uc_aux_task_state = AUX_TASK_CALC_PITCH;	

			break;

			case AUX_TASK_CALC_PITCH:

				calculate_pitch(p_global_setting);
				uc_aux_task_state = AUX_TASK_LFO;

			break;
	
			case AUX_TASK_LFO:
				
				lfo(p_global_setting);				
				uc_aux_task_state = AUX_TASK_MIDI;

			break;

			case AUX_TASK_MIDI:
					
				//If there are midi messages in the incoming message fifo, handle them.
				if(g_uc_midi_messages_in_incoming_fifo > 0)
				{
					get_midi_message_from_incoming_fifo(p_mm_incoming_message);
					midi_interpret_incoming_message(p_mm_incoming_message, p_global_setting);
				}
				
				uc_aux_task_state = AUX_TASK_SPI;

			break;

			default:

			break;


			}//Case statement end

		//clear the slow interrupt flag
		g_uc_slow_interrupt_flag = 0;
	}
	
	
  }//for Loop end

  return 0;
  }












