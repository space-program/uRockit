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


#ifndef FILTER_H
#define FILTER_H

/*definitions*/
#define MIN_FILTER_VALUE	0
#define MAX_FILTER_VALUE	512
#define MAX_FILTER_ADSR_STEP		128
#define NUMBER_OF_FILTER_ADSR_STEPS 128
#define LOG_NUMBER_OF_FILTER_ADSR_STEPS	7

//ADSR States
enum
{
	ATTACK_STATE = 0,
	DECAY_STATE,
	SUSTAIN_STATE,
	RELEASE_STATE
};

//filter state machine states
#define FREQUENCY_1_UPDATE	0
#define FREQUENCY_2_UPDATE	1
#define FILTER_Q_UPDATE		2
#define WAIT				3

//filter enable constants - the pin that the digital pot enable pin is connected to
#define FILTER_PORT 	 PORTD
#define FREQUENCY_SEL    1
#define FILTER_Q_SEL     4

/*function prototypes*/
void filter(g_setting *p_global_setting);

#endif /*FILTER_H*/
