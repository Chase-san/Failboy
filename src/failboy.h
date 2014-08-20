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
#ifndef _FAILBOY_H_
#define _FAILBOY_H_

#include <stdint.h>

#define HIBYTE(a)	((a)>>8)
#define LOBYTE(a)	((a)&0xff)

typedef uint8_t (*read_f)(uint16_t);
typedef void (*write_f)(uint16_t, uint8_t);

/* mem.c */
void mem_alloc();
void mem_free();

extern uint8_t *oam;
extern uint8_t *vram;

uint8_t read(uint16_t);
void write(uint16_t, uint8_t);

/* cpu.c */
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


#endif /* _FAILBOY_H_ */
