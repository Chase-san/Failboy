/**
 * This file is part of Failboy, a Gameboy Emulator
 * Copyright (c) Robert Maupin <chasesan@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "failboy.h"
#include <stdio.h>

enum {
	IO_P1 = 0xFF00,
	IO_SB = 0xFF01,
	IO_SC = 0xFF02,
	IO_DIV = 0xFF04,
	IO_TIMA = 0xFF05,
	IO_TMA = 0xFF06,
	IO_TAC = 0xFF07,
	IO_IF = 0xFF0F,
	IO_DMA = 0xFF46,
	IO_IE = 0xFFFF
};

static uint8_t sb = 0;

uint8_t io_read(uint16_t address) {
	return 0;
}

void io_write(uint16_t address, uint8_t value) {
	switch(address) {
		/* link cable for console ! :D */
		case IO_SB:
			sb = value;
			break;
		case IO_SC:
			/* transfer the data */
			if(value == 0x81) {
				printf("%c", sb);
			}
			break;
		default:
			//printf("WRITE %02x -> %04x\n", value, address);
			break;
	}
}
