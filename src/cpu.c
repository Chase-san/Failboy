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
} registers_t;

#define next8(r) (read(r->PC++))
#define next16(r) ((next8(r)) | (next8(r) << 8))

static uint8_t t8;
static uint16_t t16;
static uint32_t t32;

void nop(registers_t *r) { }

/* **************************************** */
/* 8-bit loads */
static inline void LD_A_n(registers_t *r) { r->A = next8(r); }
static inline void LD_B_n(registers_t *r) { r->B = next8(r); }
static inline void LD_C_n(registers_t *r) { r->C = next8(r); }
static inline void LD_D_n(registers_t *r) { r->D = next8(r); }
static inline void LD_E_n(registers_t *r) { r->E = next8(r); }
static inline void LD_H_n(registers_t *r) { r->H = next8(r); }
static inline void LD_L_n(registers_t *r) { r->L = next8(r); }
static inline void LD_aHL_n(registers_t *r) { write(r->HL, next8(r)); }

static inline void LD_A_A(registers_t *r) { r->A = r->A; }
static inline void LD_A_B(registers_t *r) { r->A = r->B; }
static inline void LD_A_C(registers_t *r) { r->A = r->C; }
static inline void LD_A_D(registers_t *r) { r->A = r->D; }
static inline void LD_A_E(registers_t *r) { r->A = r->E; }
static inline void LD_A_H(registers_t *r) { r->A = r->H; }
static inline void LD_A_L(registers_t *r) { r->A = r->L; }
static inline void LD_A_aHL(registers_t *r) { r->A = read(r->HL); }

static inline void LD_A_aC(registers_t *r) { r->A = read(r->C + 0xFF00); }
static inline void LD_aC_A(registers_t *r) { write(r->C + 0xFF00, r->A); }

static inline void LD_A_aBC(registers_t *r) { r->A = read(r->BC); }
static inline void LD_A_aDE(registers_t *r) { r->A = read(r->DE); }
static inline void LD_A_ann(registers_t *r) { r->A = read(next16(r)); }

static inline void LD_B_A(registers_t *r) { r->B = r->A; }
static inline void LD_B_B(registers_t *r) { r->B = r->B; }
static inline void LD_B_C(registers_t *r) { r->B = r->C; }
static inline void LD_B_D(registers_t *r) { r->B = r->D; }
static inline void LD_B_E(registers_t *r) { r->B = r->E; }
static inline void LD_B_H(registers_t *r) { r->B = r->H; }
static inline void LD_B_L(registers_t *r) { r->B = r->L; }
static inline void LD_B_aHL(registers_t *r) { r->B = read(r->HL); }

static inline void LD_C_A(registers_t *r) { r->C = r->A; }
static inline void LD_C_B(registers_t *r) { r->C = r->B; }
static inline void LD_C_C(registers_t *r) { r->C = r->C; }
static inline void LD_C_D(registers_t *r) { r->C = r->D; }
static inline void LD_C_E(registers_t *r) { r->C = r->E; }
static inline void LD_C_H(registers_t *r) { r->C = r->H; }
static inline void LD_C_L(registers_t *r) { r->C = r->L; }
static inline void LD_C_aHL(registers_t *r) { r->C = read(r->HL); }

static inline void LD_D_A(registers_t *r) { r->D = r->A; }
static inline void LD_D_B(registers_t *r) { r->D = r->B; }
static inline void LD_D_C(registers_t *r) { r->D = r->C; }
static inline void LD_D_D(registers_t *r) { r->D = r->D; }
static inline void LD_D_E(registers_t *r) { r->D = r->E; }
static inline void LD_D_H(registers_t *r) { r->D = r->H; }
static inline void LD_D_L(registers_t *r) { r->D = r->L; }
static inline void LD_D_aHL(registers_t *r) { r->D = read(r->HL); }

static inline void LD_E_A(registers_t *r) { r->E = r->A; }
static inline void LD_E_B(registers_t *r) { r->E = r->B; }
static inline void LD_E_C(registers_t *r) { r->E = r->C; }
static inline void LD_E_D(registers_t *r) { r->E = r->D; }
static inline void LD_E_E(registers_t *r) { r->E = r->E; }
static inline void LD_E_H(registers_t *r) { r->E = r->H; }
static inline void LD_E_L(registers_t *r) { r->E = r->L; }
static inline void LD_E_aHL(registers_t *r) { r->E = read(r->HL); }

static inline void LD_H_A(registers_t *r) { r->H = r->A; }
static inline void LD_H_B(registers_t *r) { r->H = r->B; }
static inline void LD_H_C(registers_t *r) { r->H = r->C; }
static inline void LD_H_D(registers_t *r) { r->H = r->D; }
static inline void LD_H_E(registers_t *r) { r->H = r->E; }
static inline void LD_H_H(registers_t *r) { r->H = r->H; }
static inline void LD_H_L(registers_t *r) { r->H = r->L; }
static inline void LD_H_aHL(registers_t *r) { r->H = read(r->HL); }

static inline void LD_L_A(registers_t *r) { r->L = r->A; }
static inline void LD_L_B(registers_t *r) { r->L = r->B; }
static inline void LD_L_C(registers_t *r) { r->L = r->C; }
static inline void LD_L_D(registers_t *r) { r->L = r->D; }
static inline void LD_L_E(registers_t *r) { r->L = r->E; }
static inline void LD_L_H(registers_t *r) { r->L = r->H; }
static inline void LD_L_L(registers_t *r) { r->L = r->L; }
static inline void LD_L_aHL(registers_t *r) { r->L = read(r->HL); }

static inline void LD_aHL_A(registers_t *r) { write(r->HL, r->A); }
static inline void LD_aHL_B(registers_t *r) { write(r->HL, r->B); }
static inline void LD_aHL_C(registers_t *r) { write(r->HL, r->C); }
static inline void LD_aHL_D(registers_t *r) { write(r->HL, r->D); }
static inline void LD_aHL_E(registers_t *r) { write(r->HL, r->E); }
static inline void LD_aHL_H(registers_t *r) { write(r->HL, r->H); }
static inline void LD_aHL_L(registers_t *r) { write(r->HL, r->L); }

static inline void LD_aBC_A(registers_t *r) { write(r->BC, r->A); }
static inline void LD_aDE_A(registers_t *r) { write(r->DE, r->A); }
static inline void LD_ann_A(registers_t *r) { write(next16(r), r->A); }

static inline void LDD_A_aHL(registers_t *r) { r->A = read(r->HL--); }
static inline void LDD_aHL_A(registers_t *r) { write(r->HL--, r->A); }

static inline void LDI_A_aHL(registers_t *r) { r->A = read(r->HL++); }
static inline void LDI_aHL_A(registers_t *r) { write(r->HL++, r->A); }

static inline void LDH_A_an(registers_t *r) { r->A = read(0xFF00 + next8(r)); }
static inline void LDH_an_A(registers_t *r) { write(0xFF00 + next8(r), r->A); }


/* **************************************** */
/* 16-bit loads */

static inline void LD_BC_nn(registers_t *r) { r->BC = next16(r); }
static inline void LD_DE_nn(registers_t *r) { r->DE = next16(r); }
static inline void LD_HL_nn(registers_t *r) { r->HL = next16(r); }
static inline void LD_SP_nn(registers_t *r) { r->SP = next16(r); }

static inline void LD_SP_HL(registers_t *r) { r->SP = r->HL; }

static inline void LDHL_SP_n(registers_t *r) {
	/* lovely flags */
	t8 = next8(r);
	r->HL = r->SP + (int8_t)t8;
	r->F = 0;
	r->F_H = ((r->SP ^ t8 ^ r->HL) & 0x10) == 0x10;
	r->F_C = ((r->SP ^ t8 ^ r->HL) & 0x100) == 0x100;
}

static inline void LD_ann_SP(registers_t *r) { write16(next16(r), r->SP); }

static inline void PUSH_AF(registers_t *r) {
	write16(r->SP, r->AF);
	r->SP -= 2;
}
static inline void PUSH_BC(registers_t *r) {
	write16(r->SP, r->BC);
	r->SP -= 2;
}
static inline void PUSH_DE(registers_t *r) {
	write16(r->SP, r->DE);
	r->SP -= 2;
}
static inline void PUSH_HL(registers_t *r) {
	write16(r->SP, r->HL);
	r->SP -= 2;
}

static inline void POP_AF(registers_t *r) {
	r->AF = read16(r->SP);
	r->SP += 2;
}
static inline void POP_BC(registers_t *r) {
	r->BC = read16(r->SP);
	r->SP += 2;
}
static inline void POP_DE(registers_t *r) {
	r->DE = read16(r->SP);
	r->SP += 2;
}
static inline void POP_HL(registers_t *r) {
	r->HL = read16(r->SP);
	r->SP += 2;
}

/* **************************************** */
/* 8-bit Arithmetic (ALU8) */

/* These are a PITA because of all the flags */
static inline void ADD(registers_t *r, uint8_t n) {
	t16 = r->A + n;
	r->F = 0;
	r->F_N = 0;
	r->F_Z = t16 == 0;
	r->F_H = ((r->A ^ n ^ t16) & 0x10) == 0x10;
	r->F_C = ((r->A ^ n ^ t16) & 0x100) == 0x100;
	r->A = t16;
}
static inline void ADD_A_A(registers_t *r) { ADD(r, r->A); }
static inline void ADD_A_B(registers_t *r) { ADD(r, r->B); }
static inline void ADD_A_C(registers_t *r) { ADD(r, r->C); }
static inline void ADD_A_D(registers_t *r) { ADD(r, r->D); }
static inline void ADD_A_E(registers_t *r) { ADD(r, r->E); }
static inline void ADD_A_H(registers_t *r) { ADD(r, r->H); }
static inline void ADD_A_L(registers_t *r) { ADD(r, r->L); }
static inline void ADD_A_aHL(registers_t *r) { ADD(r, read(r->HL)); }
static inline void ADD_A_n(registers_t *r) { ADD(r, next8(r)); }

static inline void ADC_A_A(registers_t *r) { ADD(r, r->A + r->F_C); }
static inline void ADC_A_B(registers_t *r) { ADD(r, r->B + r->F_C); }
static inline void ADC_A_C(registers_t *r) { ADD(r, r->C + r->F_C); }
static inline void ADC_A_D(registers_t *r) { ADD(r, r->D + r->F_C); }
static inline void ADC_A_E(registers_t *r) { ADD(r, r->E + r->F_C); }
static inline void ADC_A_H(registers_t *r) { ADD(r, r->H + r->F_C); }
static inline void ADC_A_L(registers_t *r) { ADD(r, r->L + r->F_C); }
static inline void ADC_A_aHL(registers_t *r) { ADD(r, read(r->HL) + r->F_C); }
static inline void ADC_A_n(registers_t *r) { ADD(r, next8(r) + r->F_C); }


static inline void SUB(registers_t *r, uint8_t n) {
	t16 = r->A - n;
	r->F = 0;
	r->F_N = 1;
	r->F_Z = t16 == 0;
	r->F_H = (r->A & 0xF) >= (n & 0xF);
	r->F_C = r->A >= n;
	r->A = t16;
}

static inline void SUB_A(registers_t *r) { SUB(r, r->A); }
static inline void SUB_B(registers_t *r) { SUB(r, r->B); }
static inline void SUB_C(registers_t *r) { SUB(r, r->C); }
static inline void SUB_D(registers_t *r) { SUB(r, r->D); }
static inline void SUB_E(registers_t *r) { SUB(r, r->E); }
static inline void SUB_H(registers_t *r) { SUB(r, r->H); }
static inline void SUB_L(registers_t *r) { SUB(r, r->L); }
static inline void SUB_aHL(registers_t *r) { SUB(r, read(r->HL)); }
static inline void SUB_n(registers_t *r) { SUB(r, next8(r)); }

static inline void SBC_A_A(registers_t *r) { SUB(r, r->A + r->F_C); }
static inline void SBC_A_B(registers_t *r) { SUB(r, r->B + r->F_C); }
static inline void SBC_A_C(registers_t *r) { SUB(r, r->C + r->F_C); }
static inline void SBC_A_D(registers_t *r) { SUB(r, r->D + r->F_C); }
static inline void SBC_A_E(registers_t *r) { SUB(r, r->E + r->F_C); }
static inline void SBC_A_H(registers_t *r) { SUB(r, r->H + r->F_C); }
static inline void SBC_A_L(registers_t *r) { SUB(r, r->L + r->F_C); }
static inline void SBC_A_aHL(registers_t *r) { SUB(r, read(r->HL) + r->F_C); }
static inline void SBC_A_n(registers_t *r) { SUB(r, next8(r) + r->F_C); }

static inline void AND(registers_t *r, uint8_t n) {
	r->A &= n;
	r->F = 0;
	r->F_H = 1;
	r->F_Z = r->A == 0;
}
static inline void AND_A(registers_t *r) { AND(r, r->A); }
static inline void AND_B(registers_t *r) { AND(r, r->B); }
static inline void AND_C(registers_t *r) { AND(r, r->C); }
static inline void AND_D(registers_t *r) { AND(r, r->D); }
static inline void AND_E(registers_t *r) { AND(r, r->E); }
static inline void AND_H(registers_t *r) { AND(r, r->H); }
static inline void AND_L(registers_t *r) { AND(r, r->L); }
static inline void AND_aHL(registers_t *r) { AND(r, read(r->HL)); }
static inline void AND_n(registers_t *r) { AND(r, next8(r)); }

static inline void OR(registers_t *r, uint8_t n) {
	r->A |= n;
	r->F = 0;
	r->F_Z = r->A == 0;
}
static inline void OR_A(registers_t *r) { OR(r, r->A); }
static inline void OR_B(registers_t *r) { OR(r, r->B); }
static inline void OR_C(registers_t *r) { OR(r, r->C); }
static inline void OR_D(registers_t *r) { OR(r, r->D); }
static inline void OR_E(registers_t *r) { OR(r, r->E); }
static inline void OR_H(registers_t *r) { OR(r, r->H); }
static inline void OR_L(registers_t *r) { OR(r, r->L); }
static inline void OR_aHL(registers_t *r) { OR(r, read(r->HL)); }
static inline void OR_n(registers_t *r) { OR(r, next8(r)); }

static inline void XOR(registers_t *r, uint8_t n) {
	r->A |= n;
	r->F = 0;
	r->F_Z = r->A == 0;
}
static inline void XOR_A(registers_t *r) { XOR(r, r->A); }
static inline void XOR_B(registers_t *r) { XOR(r, r->B); }
static inline void XOR_C(registers_t *r) { XOR(r, r->C); }
static inline void XOR_D(registers_t *r) { XOR(r, r->D); }
static inline void XOR_E(registers_t *r) { XOR(r, r->E); }
static inline void XOR_H(registers_t *r) { XOR(r, r->H); }
static inline void XOR_L(registers_t *r) { XOR(r, r->L); }
static inline void XOR_aHL(registers_t *r) { XOR(r, read(r->HL)); }
static inline void XOR_n(registers_t *r) { XOR(r, next8(r)); }

static inline void CP(registers_t *r, uint8_t n) {
	t16 = r->A - n;
	r->F = 0;
	r->F_N = 1;
	r->F_Z = t16 == 0;
	r->F_H = (r->A & 0xF) >= (n & 0xF);
	r->F_C = r->A >= n;
}
static inline void CP_A(registers_t *r) { CP(r, r->A); }
static inline void CP_B(registers_t *r) { CP(r, r->B); }
static inline void CP_C(registers_t *r) { CP(r, r->C); }
static inline void CP_D(registers_t *r) { CP(r, r->D); }
static inline void CP_E(registers_t *r) { CP(r, r->E); }
static inline void CP_H(registers_t *r) { CP(r, r->H); }
static inline void CP_L(registers_t *r) { CP(r, r->L); }
static inline void CP_aHL(registers_t *r) { CP(r, read(r->HL)); }
static inline void CP_n(registers_t *r) { CP(r, next8(r)); }

static inline void INC(registers_t *r, uint8_t n) {
	r->F_N = 0;
	r->F_H = (n & 0xf) == 0;
	r->F_Z = n == 0;
}
static inline void INC_A(registers_t *r) { INC(r, ++r->A); }
static inline void INC_B(registers_t *r) { INC(r, ++r->B); }
static inline void INC_C(registers_t *r) { INC(r, ++r->C); }
static inline void INC_D(registers_t *r) { INC(r, ++r->D); }
static inline void INC_E(registers_t *r) { INC(r, ++r->E); }
static inline void INC_H(registers_t *r) { INC(r, ++r->H); }
static inline void INC_L(registers_t *r) { INC(r, ++r->L); }
static inline void INC_aHL(registers_t *r) { t8 = read(r->HL) + 1; write(r->HL, t8); INC(r, t8); }

static inline void DEC(registers_t *r, uint8_t n) {
	r->F_N = 1;
	r->F_H = (n & 0xf) != 0xf;
	r->F_Z = n == 0;
}
static inline void DEC_A(registers_t *r) { DEC(r, ++r->A); }
static inline void DEC_B(registers_t *r) { DEC(r, ++r->B); }
static inline void DEC_C(registers_t *r) { DEC(r, ++r->C); }
static inline void DEC_D(registers_t *r) { DEC(r, ++r->D); }
static inline void DEC_E(registers_t *r) { DEC(r, ++r->E); }
static inline void DEC_H(registers_t *r) { DEC(r, ++r->H); }
static inline void DEC_L(registers_t *r) { DEC(r, ++r->L); }
static inline void DEC_aHL(registers_t *r) { t8 = read(r->HL) + 1; write(r->HL, t8); DEC(r, t8); }


/* **************************************** */
/* 16-bit Arithmetic (ALU16) */
static inline void ADD_HL(registers_t *r, uint16_t n) {
	t32 = r->HL + n;
	r->F_N = 0;
	r->F_H = ((r->HL ^ n ^ t32) & 0x1000) == 0x1000;
	r->F_C = ((r->HL ^ n ^ t32) & 0x10000) == 0x10000;
	r->HL = t32;
}
static inline void ADD_HL_BC(registers_t *r) { ADD_HL(r,r->BC); }
static inline void ADD_HL_DE(registers_t *r) { ADD_HL(r,r->DE); }
static inline void ADD_HL_HL(registers_t *r) { ADD_HL(r,r->HL); }
static inline void ADD_HL_SP(registers_t *r) { ADD_HL(r,r->SP); }


/* **************************************** */

static inline void HALT(registers_t *r) { }

static const instruction_f instrmap[256] = {
	nop, LD_BC_nn, LD_aBC_A, nop,	INC_B, DEC_B, LD_B_n, nop,	/* 00, 04 */
	LD_ann_SP, ADD_HL_BC, LD_A_aBC, nop,	INC_C, DEC_C, LD_C_n, nop,	/* 08, 0c */
	nop, LD_DE_nn, LD_aDE_A, nop,	INC_D, DEC_D, LD_D_n, nop,	/* 10, 14 */
	nop, ADD_HL_DE, LD_A_aDE, nop,	INC_E, DEC_E, LD_E_n, nop,	/* 18, 1c */
	nop, LD_HL_nn, LDI_aHL_A, nop,	INC_H, DEC_H, LD_H_n, nop,	/* 20, 24 */
	nop, ADD_HL_HL, LDI_A_aHL, nop,	INC_L, DEC_L, LD_L_n, nop,	/* 28, 2c */
	nop, LD_SP_nn, LDD_aHL_A, nop,	INC_aHL, DEC_aHL, LD_aHL_n, nop,	/* 30, 34 */
	nop, ADD_HL_SP, LDD_A_aHL, nop,	INC_A, DEC_A, LD_A_n, nop,	/* 38, 3c */
	LD_B_B, LD_B_C, LD_B_D, LD_B_E,	LD_B_H, LD_B_L, LD_B_aHL, LD_B_A,	/* 40, 44 */
	LD_C_B, LD_C_C, LD_C_D, LD_C_E,	LD_C_H, LD_C_L, LD_C_aHL, LD_C_A,	/* 48, 4c */
	LD_D_B, LD_D_C, LD_D_D, LD_D_E,	LD_D_H, LD_D_L, LD_D_aHL, LD_D_A,	/* 50, 54 */
	LD_E_B, LD_E_C, LD_E_D, LD_E_E,	LD_E_H, LD_E_L, LD_E_aHL, LD_E_A,	/* 50, 54 */
	LD_H_B, LD_H_C, LD_H_D, LD_H_E,	LD_H_H, LD_H_L, LD_H_aHL, LD_H_A,	/* 60, 64 */
	LD_L_B, LD_L_C, LD_L_D, LD_L_E,	LD_L_H, LD_L_L, LD_L_aHL, LD_L_A,	/* 68, 6c */
	LD_aHL_B, LD_aHL_C, LD_aHL_D, LD_aHL_E,	LD_aHL_H, LD_aHL_L, HALT, LD_aHL_A,	/* 70, 74 */
	LD_A_B, LD_A_C, LD_A_D, LD_A_E,	LD_A_H, LD_A_L, LD_A_aHL, LD_A_A,	/* 78, 7c */
	ADD_A_B, ADD_A_C, ADD_A_D, ADD_A_E,	ADD_A_H, ADD_A_L, ADD_A_aHL, ADD_A_A,	/* 80, 84 */
	ADC_A_B, ADC_A_C, ADC_A_D, ADC_A_E,	ADC_A_H, ADC_A_L, ADC_A_aHL, ADC_A_A,	/* 88, 8c */
	SUB_B, SUB_C, SUB_D, SUB_E,	SUB_H, SUB_L, SUB_aHL, SUB_A,	/* 90, 94 */
	SBC_A_B, SBC_A_C, SBC_A_D, SBC_A_E,	SBC_A_H, SBC_A_L, SBC_A_aHL, SBC_A_A,	/* 98, 9c */
	AND_B, AND_C, AND_D, AND_E,	AND_H, AND_L, AND_aHL, AND_A,	/* a0, a4 */
	XOR_B, XOR_C, XOR_D, XOR_E,	XOR_H, XOR_L, XOR_aHL, XOR_A,	/* a8, ac */
	OR_B, OR_C, OR_D, OR_E,	OR_H, OR_L, OR_aHL, OR_A,	/* b0, b4 */
	CP_B, CP_C, CP_D, CP_E,	CP_H, CP_L, CP_aHL, CP_A,	/* b8, bc */
	nop, POP_BC, nop, nop,	nop, PUSH_BC, ADD_A_n, nop,	/* c0, c4 */
	nop, nop, nop, nop,	nop, nop, ADC_A_n, nop,	/* c8, cc */
	nop, POP_DE, nop, nop,	nop, PUSH_DE, SUB_n, nop,	/* d0, d4 */
	nop, nop, nop, nop,	nop, nop, SBC_A_n, nop,	/* d8, dc */
	LDH_an_A, POP_HL, LD_aC_A, nop,	nop, PUSH_HL, AND_n, nop,	/* e0, e4 */
	nop, nop, LD_ann_A, nop,	nop, nop, XOR_n, nop,	/* e8, ec */
	LDH_A_an, POP_AF, LD_A_aC, nop,	nop, PUSH_AF, OR_n, nop,	/* f0, f4 */
	LDHL_SP_n, LD_SP_HL, LD_A_ann, nop,	nop, nop, CP_n, nop,	/* f8, fc */
};

void exec(registers_t *r, uint8_t op) { instrmap[op](r); }
