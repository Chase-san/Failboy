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

typedef struct registers {
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
} registers_t;

#define nextpc(r) (read(r->PC++))
#define next16(r) ((nextpc(r)) | (nextpc(r) << 8))

void nop(registers_t *r, uint8_t op) { }

/* **************************************** */
/* 8-bit loads */
void LD_A_n(registers_t *r, uint8_t op) { r->A = nextpc(r); }
void LD_B_n(registers_t *r, uint8_t op) { r->B = nextpc(r); }
void LD_C_n(registers_t *r, uint8_t op) { r->C = nextpc(r); }
void LD_D_n(registers_t *r, uint8_t op) { r->D = nextpc(r); }
void LD_E_n(registers_t *r, uint8_t op) { r->E = nextpc(r); }
void LD_H_n(registers_t *r, uint8_t op) { r->H = nextpc(r); }
void LD_L_n(registers_t *r, uint8_t op) { r->L = nextpc(r); }
void LD_aHL_n(registers_t *r, uint8_t op) { write(r->HL, nextpc(r)); }

void LD_A_A(registers_t *r, uint8_t op) { r->A = r->A; }
void LD_A_B(registers_t *r, uint8_t op) { r->A = r->B; }
void LD_A_C(registers_t *r, uint8_t op) { r->A = r->C; }
void LD_A_D(registers_t *r, uint8_t op) { r->A = r->D; }
void LD_A_E(registers_t *r, uint8_t op) { r->A = r->E; }
void LD_A_H(registers_t *r, uint8_t op) { r->A = r->H; }
void LD_A_L(registers_t *r, uint8_t op) { r->A = r->L; }
void LD_A_aHL(registers_t *r, uint8_t op) { r->A = read(r->HL); } /* LD A, (HL) */

void LD_A_aC(registers_t *r, uint8_t op) { r->A = read(r->C + 0xFF00); } /* LD A, (FF00+C) */
void LD_aC_A(registers_t *r, uint8_t op) { write(r->C + 0xFF00, r->A); } /* LD (FF00+C), A */

void LD_A_aBC(registers_t *r, uint8_t op) { r->A = read(r->BC); } /* LD A, (BC) */
void LD_A_aDE(registers_t *r, uint8_t op) { r->A = read(r->DE); } /* LD A, (DE) */
void LD_A_ann(registers_t *r, uint8_t op) { r->A = read(next16(r)); } /* LD A, (nn) */

void LD_B_A(registers_t *r, uint8_t op) { r->B = r->A; }
void LD_B_B(registers_t *r, uint8_t op) { r->B = r->B; }
void LD_B_C(registers_t *r, uint8_t op) { r->B = r->C; }
void LD_B_D(registers_t *r, uint8_t op) { r->B = r->D; }
void LD_B_E(registers_t *r, uint8_t op) { r->B = r->E; }
void LD_B_H(registers_t *r, uint8_t op) { r->B = r->H; }
void LD_B_L(registers_t *r, uint8_t op) { r->B = r->L; }
void LD_B_aHL(registers_t *r, uint8_t op) { r->B = read(r->HL); } /* LD B, (HL) */

void LD_C_A(registers_t *r, uint8_t op) { r->C = r->A; }
void LD_C_B(registers_t *r, uint8_t op) { r->C = r->B; }
void LD_C_C(registers_t *r, uint8_t op) { r->C = r->C; }
void LD_C_D(registers_t *r, uint8_t op) { r->C = r->D; }
void LD_C_E(registers_t *r, uint8_t op) { r->C = r->E; }
void LD_C_H(registers_t *r, uint8_t op) { r->C = r->H; }
void LD_C_L(registers_t *r, uint8_t op) { r->C = r->L; }
void LD_C_aHL(registers_t *r, uint8_t op) { r->C = read(r->HL); } /* LD C, (HL) */

void LD_D_A(registers_t *r, uint8_t op) { r->D = r->A; }
void LD_D_B(registers_t *r, uint8_t op) { r->D = r->B; }
void LD_D_C(registers_t *r, uint8_t op) { r->D = r->C; }
void LD_D_D(registers_t *r, uint8_t op) { r->D = r->D; }
void LD_D_E(registers_t *r, uint8_t op) { r->D = r->E; }
void LD_D_H(registers_t *r, uint8_t op) { r->D = r->H; }
void LD_D_L(registers_t *r, uint8_t op) { r->D = r->L; }
void LD_D_aHL(registers_t *r, uint8_t op) { r->D = read(r->HL); } /* LD C, (HL) */

void LD_E_A(registers_t *r, uint8_t op) { r->E = r->A; }
void LD_E_B(registers_t *r, uint8_t op) { r->E = r->B; }
void LD_E_C(registers_t *r, uint8_t op) { r->E = r->C; }
void LD_E_D(registers_t *r, uint8_t op) { r->E = r->D; }
void LD_E_E(registers_t *r, uint8_t op) { r->E = r->E; }
void LD_E_H(registers_t *r, uint8_t op) { r->E = r->H; }
void LD_E_L(registers_t *r, uint8_t op) { r->E = r->L; }
void LD_E_aHL(registers_t *r, uint8_t op) { r->E = read(r->HL); } /* LD C, (HL) */

void LD_H_A(registers_t *r, uint8_t op) { r->H = r->A; }
void LD_H_B(registers_t *r, uint8_t op) { r->H = r->B; }
void LD_H_C(registers_t *r, uint8_t op) { r->H = r->C; }
void LD_H_D(registers_t *r, uint8_t op) { r->H = r->D; }
void LD_H_E(registers_t *r, uint8_t op) { r->H = r->E; }
void LD_H_H(registers_t *r, uint8_t op) { r->H = r->H; }
void LD_H_L(registers_t *r, uint8_t op) { r->H = r->L; }
void LD_H_aHL(registers_t *r, uint8_t op) { r->H = read(r->HL); } /* LD C, (HL) */

void LD_L_A(registers_t *r, uint8_t op) { r->L = r->A; }
void LD_L_B(registers_t *r, uint8_t op) { r->L = r->B; }
void LD_L_C(registers_t *r, uint8_t op) { r->L = r->C; }
void LD_L_D(registers_t *r, uint8_t op) { r->L = r->D; }
void LD_L_E(registers_t *r, uint8_t op) { r->L = r->E; }
void LD_L_H(registers_t *r, uint8_t op) { r->L = r->H; }
void LD_L_L(registers_t *r, uint8_t op) { r->L = r->L; }
void LD_L_aHL(registers_t *r, uint8_t op) { r->L = read(r->HL); } /* LD C, (HL) */

 /* LD (HL), nn, where nn is a register */
void LD_aHL_A(registers_t *r, uint8_t op) { write(r->HL, r->A); }
void LD_aHL_B(registers_t *r, uint8_t op) { write(r->HL, r->B); }
void LD_aHL_C(registers_t *r, uint8_t op) { write(r->HL, r->C); }
void LD_aHL_D(registers_t *r, uint8_t op) { write(r->HL, r->D); }
void LD_aHL_E(registers_t *r, uint8_t op) { write(r->HL, r->E); }
void LD_aHL_H(registers_t *r, uint8_t op) { write(r->HL, r->H); }
void LD_aHL_L(registers_t *r, uint8_t op) { write(r->HL, r->L); }
void LD_aHL_aHL(registers_t *r, uint8_t op) { r->L = read(r->HL); }

void LD_aBC_A(registers_t *r, uint8_t op) { write(r->BC, r->A); }
void LD_aDE_A(registers_t *r, uint8_t op) { write(r->DE, r->A); }
void LD_ann_A(registers_t *r, uint8_t op) { write(next16(r), r->A); }



/* **************************************** */

void HALT(registers_t *r, uint8_t op) { }

static const instruction_f instrmap[256] = {
	nop,nop,LD_aBC_A,nop,	nop,nop,LD_B_n,nop,	/* 00, 04 */
	nop,nop,LD_A_aBC,nop,	nop,nop,LD_C_n,nop,	/* 08, 0c */
	nop,nop,LD_aDE_A,nop,	nop,nop,LD_D_n,nop,	/* 10, 14 */
	nop,nop,LD_A_aDE,nop,	nop,nop,LD_E_n,nop,	/* 18, 1c */
	nop,nop,nop,nop,	nop,nop,LD_H_n,nop,	/* 20, 24 */
	nop,nop,nop,nop,	nop,nop,LD_L_n,nop,	/* 28, 2c */
	nop,nop,nop,nop,	nop,nop,LD_aHL_n,nop,	/* 30, 34 */
	nop,nop,nop,nop,	nop,nop,LD_A_n,nop,	/* 38, 3c */
	LD_B_B,LD_B_C,LD_B_D,LD_B_E,	LD_B_H,LD_B_L,LD_B_aHL,LD_B_A,	/* 40, 44 */
	LD_C_B,LD_C_C,LD_C_D,LD_C_E,	LD_C_H,LD_C_L,LD_C_aHL,LD_C_A,	/* 48, 4c */
	LD_D_B,LD_D_C,LD_D_D,LD_D_E,	LD_D_H,LD_D_L,LD_D_aHL,LD_D_A,	/* 50, 54 */
	LD_E_B,LD_E_C,LD_E_D,LD_E_E,	LD_E_H,LD_E_L,LD_E_aHL,LD_E_A,	/* 50, 54 */
	LD_H_B,LD_H_C,LD_H_D,LD_H_E,	LD_H_H,LD_H_L,LD_H_aHL,LD_H_A,	/* 60, 64 */
	LD_L_B,LD_L_C,LD_L_D,LD_L_E,	LD_L_H,LD_L_L,LD_L_aHL,LD_L_A,	/* 68, 6c */
	LD_aHL_B,LD_aHL_C,LD_aHL_D,LD_aHL_E,	LD_aHL_H,LD_aHL_L,HALT,LD_aHL_A,	/* 70, 74 */
	LD_A_B,LD_A_C,LD_A_D,LD_A_E,	LD_A_H,LD_A_L,LD_A_aHL,LD_A_A,	/* 78, 7c */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* 80, 84 */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* 88, 8c */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* 90, 94 */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* 98, 9c */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* a0, a4 */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* a8, ac */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* b0, b4 */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* b8, bc */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* c0, c4 */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* c8, cc */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* d0, d4 */
	nop,nop,nop,nop,	nop,nop,nop,nop,	/* d8, dc */
	nop,nop,LD_aC_A,nop,	nop,nop,nop,nop,	/* e0, e4 */
	nop,nop,LD_ann_A,nop,	nop,nop,nop,nop,	/* e8, ec */
	nop,nop,LD_A_aC,nop,	nop,nop,nop,nop,	/* f0, f4 */
	nop,nop,LD_A_ann,nop,	nop,nop,nop,nop,	/* f8, fc */
};

void exec(registers_t *r, uint8_t op) {
	instrmap[op](r,op);
}
