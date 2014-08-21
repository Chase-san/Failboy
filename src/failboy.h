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

/* cart.c */
void cart_load(const char *);
void cart_free();

/* mem.c */
void mem_alloc();
void mem_free();

extern uint8_t *oam;
extern uint8_t *vram;

uint8_t read(uint16_t);
uint16_t read16(uint16_t);

void write(uint16_t, uint8_t);
void write16(uint16_t, uint16_t);

/* cpu.c */
struct registers {
	uint16_t PC;
	union {
		uint16_t SP;
		struct {
			uint8_t SPLO;
			uint8_t SPHI;
		};
	};
	union {
		uint16_t AF;
		struct {
			union {
				uint8_t F;
				struct {
					uint8_t : 4; /* 0-3 unused */
					uint8_t F_C : 1; /* carry */
					uint8_t F_H : 1; /* half-carry (BCD) */
					uint8_t F_N : 1; /* add/sub flag (BCD) */
					uint8_t F_Z : 1; /* zero flag */
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
};

extern struct registers r;
extern uint32_t cycle_counter;

#define rpc8() (read(r.PC++))
#define rpc16() ((rpc8()) | (rpc8() << 8))

void cpu_bios_init();
void step();

#endif /* _FAILBOY_H_ */
