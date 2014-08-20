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

typedef struct {
	uint16_t PC;
	uint16_t SP;
	union {
		uint16_t AF;
		struct {
			union {
				uint8_t F;
				struct {
					uint8_t : 4; /* 0-3 unused */
					uint8_t cf : 1; /* carry */
					uint8_t hf : 1; /* half-carry (BCD) */
					uint8_t nf : 1; /* add/sub flag (BCD) */
					uint8_t zf : 1; /* zero flag */
				};
			};
			uint8_t A;
		};
	};
	union {
		uint16_t BC;
		struct {
			uint8_t C;
			uint8_t B;
		};
	};
	union {
		uint16_t DE;
		struct {
			uint8_t E;
			uint8_t D;
		};
	};
	union {
		uint16_t HL;
		struct {
			uint8_t L;
			uint8_t H;
		};
	};
} regs;

/* RAM 127 bytes */
/* OAM RAM 140 bytes */

