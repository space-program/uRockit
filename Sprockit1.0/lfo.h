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

#ifndef LFO_H
#define LFO_H

#define NUMBER_OF_LFOS			1

extern const unsigned char auc_lfo_dest_decode[8];//lfo destination look-up table
extern const unsigned int g_aun_lfo_rate_lut[32];//lfo rate look-up table

void 
lfo(g_setting *p_global_setting);

#endif
