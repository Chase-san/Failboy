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

void NOP(registers_t *r) { }

#include "cpu_op.cx"
#include "cpu_op_cb.cx"

static const instruction_f instrmapCB[256] = {
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 00, 04 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 08, 0c */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 10, 14 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 18, 1c */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 20, 24 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 28, 2c */
	SWAP_B,SWAP_C,SWAP_D,SWAP_E,	SWAP_H,SWAP_L,SWAP_aHL,SWAP_A,	/* 30, 34 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 38, 3c */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 40, 44 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 48, 4c */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 50, 54 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 50, 54 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 60, 64 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 68, 6c */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 70, 74 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 78, 7c */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 80, 84 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 88, 8c */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 90, 94 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* 98, 9c */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* a0, a4 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* a8, ac */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* b0, b4 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* b8, bc */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* c0, c4 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* c8, cc */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* d0, d4 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* d8, dc */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* e0, e4 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* e8, ec */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* f0, f4 */
	NOP,NOP,NOP,NOP,	NOP,NOP,NOP,NOP,	/* f8, fc */
};

static inline void INSTRCB(registers_t *r) {
	instrmapCB[next8(r)](r);
}

static inline void XXX(registers_t *r) {  /* missing opcode */ }

static const instruction_f instrmap[256] = {
	NOP, LD_BC_nn, LD_aBC_A, INC_BC,	INC_B, DEC_B, LD_B_n, RLCA,	/* 00, 04 */
	LD_ann_SP, ADD_HL_BC, LD_A_aBC, DEC_BC,	INC_C, DEC_C, LD_C_n, RRCA,	/* 08, 0c */
	STOP, LD_DE_nn, LD_aDE_A, INC_DE,	INC_D, DEC_D, LD_D_n, RLA,	/* 10, 14 */
	JR_n, ADD_HL_DE, LD_A_aDE, DEC_DE,	INC_E, DEC_E, LD_E_n, RRA,	/* 18, 1c */
	JR_NZ_n, LD_HL_nn, LDI_aHL_A, INC_HL,	INC_H, DEC_H, LD_H_n, DAA,	/* 20, 24 */
	JR_Z_n, ADD_HL_HL, LDI_A_aHL, DEC_HL,	INC_L, DEC_L, LD_L_n, CPL,	/* 28, 2c */
	JR_NC_n, LD_SP_nn, LDD_aHL_A, INC_SP,	INC_aHL, DEC_aHL, LD_aHL_n, SCF,	/* 30, 34 */
	JR_C_n, ADD_HL_SP, LDD_A_aHL, DEC_SP,	INC_A, DEC_A, LD_A_n, CCF,	/* 38, 3c */
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
	RET_NZ, POP_BC, JP_NZ, JP,	CALL_NZ_nn, PUSH_BC, ADD_A_n, RST00,	/* c0, c4 */
	RET_Z, RET, JP_Z, INSTRCB,	CALL_Z_nn, CALL_nn, ADC_A_n, RST08,	/* c8, cc */
	RET_NC, POP_DE, JP_NC, XXX,	CALL_NC_nn, PUSH_DE, SUB_n, RST10,	/* d0, d4 */
	RET_C, RETI, JP_C, XXX,	CALL_C_nn, XXX, SBC_A_n, RST18,	/* d8, dc */
	LDH_an_A, POP_HL, LD_aC_A, XXX,	XXX, PUSH_HL, AND_n, RST20,	/* e0, e4 */
	ADD_SP_n, JP_HL, LD_ann_A, XXX,	XXX, XXX, XOR_n, RST28,	/* e8, ec */
	LDH_A_an, POP_AF, LD_A_aC, DI,	XXX, PUSH_AF, OR_n, RST30,	/* f0, f4 */
	LDHL_SP_n, LD_SP_HL, LD_A_ann, EI,	XXX, XXX, CP_n, RST38,	/* f8, fc */
};

void exec(registers_t *r, uint8_t op) {
	instrmap[op](r);
}
