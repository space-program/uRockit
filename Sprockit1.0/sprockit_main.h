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

#ifndef SPROCKIT_MAIN_H
#define SPROCKIT_MAIN_H


//Macros
#define SET_BIT(address, bit) (address |= (1<<bit))
#define CLEAR_BIT(address, bit) (address &= ~(1<<bit))
#define CHECK_BIT(address, bit) (address & (1<<bit))
#define FALSE 	0
#define TRUE	1
#define RESET_WATCHDOG MCUSR = 0
#define POT_MUX_SEL1(bit) PORTB |= (bit<<PB1)
#define POT_MUX_SEL0(bit) PORTB |= bit
#define RESULT_IS_NEGATIVE	CHECK_BIT(SREG, 2)
#define RESULT_IS_POSITIVE	!CHECK_BIT(SREG, 2)
#define RESULT_TWOS_COMPLEMENT_OVERFLOW	CHECK_BIT(SREG, 3)

//Sample rate related constants
#define SAMPLE_FREQUENCY  	    	32768	
#define SAMPLE_MAX 				    32767 //highest sample
#define THREE_QUARTER_SAMPLE_MAX	24575 //3/4 of highest sample
#define HALF_SAMPLE_MAX			    16383 //half of the highest sample
#define QUARTER_SAMPLE_MAX		    8191 //1/4 of highest sample

#define OFF	0
#define ON	1

//Waveshapes
#define SIN				0
#define	RAMP			1
#define	SQUARE			2
#define	TRIANGLE		3
#define	MORPH_1			4
#define MORPH_2			5
#define MORPH_3			6
#define MORPH_4			7
#define MORPH_5			8
#define MORPH_6			9
#define MORPH_7			10
#define MORPH_8			11
#define MORPH_9			12
#define HARD_SYNC		13
#define NOISE			14 
#define RAW_SQUARE		15

//Oscillator
#define NUMBER_OF_OSCILLATORS	2
#define NUM_OF_SAMPLES 		 	16
#define LOG_NUM_OF_SAMPLES 	 	4
#define OSC_1					0
#define OSC_2					1



//Auxilliary Task States
#define AUX_TASK_AMPLITUDE 		0
#define AUX_TASK_SPI		    0
#define AUX_TASK_READ_AD    	1
#define AUX_TASK_CALC_PITCH		2
#define AUX_TASK_LFO			3
#define AUX_TASK_MIDI			4

//Control Signal Sources
#define SOURCE_AD 			0	//Internal knob controls
#define SOURCE_LOOP			1	//Stored loop values
#define SOURCE_EXTERNAL 		2	//EEPROM recalled value or MIDI loaded values

//Knobs/Pots
#define NUMBER_OF_KNOBS				8	//The A to D has to know how many knobs to loop through
#define NUMBER_OF_MUX_KNOBS			8
#define NUMBER_OF_LOOP_KNOBS		8  //Number of knobs for the drone loop function 
#define NUMBER_OF_KNOB_PARAMETERS	8  //Number of ADs plus the LFO parameters which are like imaginary knobs
#define NUMBER_OF_PARAMETERS		32	//The total number of parameters including button set parameters
//ADSR Parameters/Knobs - these constants are used as indexes to access members of the ADSR array
#define FILTER_Q			0
#define LFO_RATE			1
#define FILTER_FREQUENCY	2
#define OSC_DETUNE			3
#define ADSR_LENGTH			4
#define LFO_AMOUNT			5
#define OSC_WAVESHAPE		6
#define ADSR_ATTACK			7

#define FILTER_SUSTAIN		8
#define FILTER_ENV_AMT		9
#define FILTER_ATTACK		10
#define OSC_MIX				11
#define LFO_SHAPE			12
#define FILTER_DECAY		13
#define	ADSR_RELEASE		14
#define OSC_1_WAVESHAPE		15
#define OSC_2_WAVESHAPE		16
#define	ADSR_SUSTAIN		17
#define FILTER_RELEASE		18
#define PITCH_SHIFT			19
#define AMPLITUDE			20
#define LFO_DEST			21
#define FILTER_TYPE			22
#define LFO_WAVESHAPE		23
#define LFO_SYNC			24
#define PORTAMENTO			25
#define ARPEGGIATOR_MODE	26
#define ARPEGGIATOR_SPEED	27
#define ARPEGGIATOR_LENGTH	28
#define ARPEGGIATOR_GATE	29
#define ADSR_DECAY			30

//SPI Related Constants and Macros
#define SPI_TX_BUF_LGTH    				3
#define CLEAR_EXT_INTERRUPTS			EIFR |= 0xFF;
#define ENABLE_EXT_INT_0   				EIMSK |= (1 << INT0)  
#define DISABLE_EXT_INT_0  				EIMSK &= ~(1 << INT0)
#define ENABLE_EXT_INT_1   				EIMSK |= (1 << INT1) 
#define DISABLE_EXT_INT_1  				EIMSK &= ~(1 << INT1)


//Global Variables - They should always be volatile and prefixed by g_.
//Global Flags
extern unsigned char g_uc_output;//main output variable
volatile unsigned char g_uc_spi_buffer;//Buffer for the Serial Peripheral Interface
volatile unsigned char g_uc_sample_request_flag;//flag used to set the output and request a new sample from the state machine
volatile unsigned char g_uc_slow_interrupt_flag;//flag set to tell events to update their values
volatile unsigned char g_uc_key_press_flag;//flag for when a key is pressed
volatile unsigned char g_uc_note_on_flag;//flag for generating audio output
volatile unsigned char g_uc_ad_ready_flag;//flag used to indicate when the AD has completed a reading
volatile unsigned char g_uc_spi_ready_flag;//flag used to indicate that the SPI has completed transmission
volatile unsigned char g_uc_lfo_midi_sync_flag;//This flag is used to sync events to the arrival of new note on messages.
volatile unsigned char g_uc_filter_envelope_sync_flag;//Syncs the filter envelope to notes being played
volatile unsigned char g_uc_oscillator_midi_sync_flag;//Syncs morphing oscillators to key press
volatile unsigned int g_un_oscillator_frequency_osc1;
volatile unsigned int g_un_oscillator_frequency_osc2;
volatile unsigned char g_uc_adsr_midi_sync_flag;
volatile unsigned char g_uc_ext_int_0_flag;//Flag indicates external interrupt 0 has been triggered
volatile unsigned char g_uc_ext_int_1_flag;//Flag indicates external interrupt 1 has been triggered
volatile unsigned char g_uc_drone_flag;

volatile unsigned int g_un_switch_debounce_timer;//Timer used to debounce switch presses


//Global Setting Type Declaration
//This structure holds all the settings information for the synth. We pass this structure to functions
//to allow them to change settings.
typedef struct
{	
		
	//oscillator variables
	unsigned int aun_sample_reference[3];	//keeps track of where we are in the cycle for each oscillator
	unsigned char auc_midi_note_index[NUMBER_OF_OSCILLATORS];//the midi index of the note frequency
	unsigned int aun_note_frequency[NUMBER_OF_OSCILLATORS];//the actual note frequency

	//ADSR variables
	unsigned char uc_adsr_multiplier;	//used for the ADSR calculation
	unsigned char uc_note_velocity;		//how hard the key was hit

	//Output amplitude
	unsigned char uc_amplitude;	//main output amplitude
	
	//LFO variables
	unsigned char uc_lfo_sel;	//which LFO is active for the rate/amount pots
	unsigned char uc_lfo_sync;

		
	//parameter storage arrays
	unsigned char auc_ad_values[NUMBER_OF_PARAMETERS];	//array containing values from the ADC
	unsigned char auc_synth_params[NUMBER_OF_PARAMETERS];//array of synth parameters affected by the ADC readings
	unsigned char auc_external_params[NUMBER_OF_PARAMETERS];//array containing parameter values loaded from eeprom
	unsigned char auc_parameter_source[NUMBER_OF_PARAMETERS];/*flag array contains the source for the synth parameters, internal or external*/

} g_setting;

extern g_setting global_setting, *p_global_setting;

#endif /*SPROCKIT_MAIN_H*/
