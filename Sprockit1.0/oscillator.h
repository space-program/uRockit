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

#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#define MORPH_1_TIME_PERIOD 50
#define MORPH_2_TIME_PERIOD	10
#define PHASE_SHIFT_TIMER_2	50

#define OSCILLATOR_1	0
#define OSCILLATOR_2	1
#define OSCILLATOR_MIX	2

const unsigned char AUC_OSCILLATOR_LUT[32][3];

void
decode_oscillator_waveshape(volatile g_setting *p_global_setting, unsigned char ucwaveshape);

unsigned char 
oscillator(unsigned char uc_waveshape, unsigned int un_sample_reference, unsigned char uc_frequency);

unsigned char
calculate_square(unsigned char uc_sample_index, unsigned char uc_frequency, unsigned char uc_pulse_width);

unsigned char 
linear_interpolate(unsigned char uc_reference, unsigned char uc_sample_1, unsigned char uc_sample_2);


#endif /*OSCILLATOR_H*/
