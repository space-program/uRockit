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

#ifndef SPI_H
#define SPI_H

#define CHIP_SELECT_MASK		0X12

//SPI Related Global Variables
volatile unsigned char g_auc_spi_tx_buffer[SPI_TX_BUF_LGTH];//This array provides a buffer for spi transmission.
volatile unsigned char *g_p_uc_spi_tx_buffer;//This pointer is used to access members of the spi tx buffer.
volatile unsigned char *g_p_uc_spi_tx_buffer_end;//This points to the end of the spi tx buffer.
volatile unsigned char g_uc_spi_tx_buffer_index;//This holds the index for spi tx buffer actions.
volatile unsigned int  g_uc_spi_enable_port;//This holds the memory mapped address of the enable port for the spi.
volatile unsigned char g_uc_spi_enable_pin;//This holds the number of the enable pin for the spi.

//Function Prototypes
void
spi(void);

unsigned char 
spi_simple_read(void);

void 
spi_simple_transmit(unsigned char uc_data);

void 
spi_ioexpander_reg_update(unsigned char uc_ioexpander_address, 
                               unsigned char uc_reg_address,
                               unsigned char uc_value);

void 
send_spi_two_bytes(unsigned char uc_byte_one, 
                        unsigned char uc_byte_two);
void 
send_spi_three_bytes(unsigned char uc_byte_one, 
                     unsigned char uc_byte_two,
					 unsigned char uc_byte_three);

void
set_button_press_flag();

void
clear_button_press_flag();

unsigned char
check_button_press_flag();

void
set_debounce_timer();


#endif
