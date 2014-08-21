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
#include "cpu_instr.h"
#include "cpu_instr_cb.h"

typedef void (*instruction_f)();
typedef void (*instruction_cb_f)(uint8_t);

struct registers r;

uint32_t cycle_counter = 0;

void NOP() { }
void XXX() { /* missing opcode */ }

static const instruction_f instr_cb_map[64] = {
	RLC_B, RLC_C, RLC_D, RLC_E, RLC_H, RLC_L, RLC_aHL, RLCA,	/* 00-07 */
	RRC_B, RRC_C, RRC_D, RRC_E, RRC_H, RRC_L, RRC_aHL, RRCA,	/* 08-0f */
	RL_B, RL_C, RL_D, RL_E, RL_H, RL_L, RL_aHL, RLA,	/* 10-17 */
	RR_B, RR_C, RR_D, RR_E, RR_H, RR_L, RR_aHL, RRA,	/* 18-1f */
	SLA_B, SLA_C, SLA_D, SLA_E, SLA_H, SLA_L, SLA_aHL, SLA_A,	/* 20-27 */
	SRA_B, SRA_C, SRA_D, SRA_E, SRA_H, SRA_L, SRA_aHL, SRA_A,	/* 28-2f */
	SWAP_B, SWAP_C, SWAP_D, SWAP_E, SWAP_H, SWAP_L, SWAP_aHL, SWAP_A,	/* 30-37 */
	SRL_B, SRL_C, SRL_D, SRL_E, SRL_H, SRL_L, SRL_aHL, SRL_A,	/* 38-3f */
};

static const instruction_cb_f instr_cb_bit_map[8] = {
	BIT_b_B, BIT_b_C, BIT_b_D, BIT_b_E, BIT_b_H, BIT_b_L, BIT_b_aHL, BIT_b_A
};
static const instruction_cb_f instr_cb_res_map[8] = {
	RES_b_B, RES_b_C, RES_b_D, RES_b_E, RES_b_H, RES_b_L, RES_b_aHL, RES_b_A
};
static const instruction_cb_f instr_cb_set_map[8] = {
	SET_b_B, SET_b_C, SET_b_D, SET_b_E, SET_b_H, SET_b_L, SET_b_aHL, SET_b_A
};

static const uint8_t instr_cb_timing[256] = {
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2
};

static inline void step_cb() {
	uint8_t op = rpc8();
	if(op < 0x40) {
		instr_cb_map[op]();
		cycle_counter += instr_cb_timing[op] << 2;
		return;
	}
	if(op < 0x80) { /* BIT */
		instr_cb_bit_map[op & 7]((op>>3)&7);
		cycle_counter += instr_cb_timing[op] << 2;
		return;
	}
	if(op < 0xC0) { /* RES */
		instr_cb_res_map[op & 7]((op>>3)&7);
		cycle_counter += instr_cb_timing[op] << 2;
		return;
	}
	/* SET */
	instr_cb_set_map[op & 7]((op>>3)&7);
	cycle_counter += instr_cb_timing[op] << 2;
}

static const instruction_f instr_map[256] = {
	NOP, LD_BC_nn, LD_aBC_A, INC_BC, INC_B, DEC_B, LD_B_n, RLCA,	/* 00-07 */
	LD_ann_SP, ADD_HL_BC, LD_A_aBC, DEC_BC, INC_C, DEC_C, LD_C_n, RRCA,	/* 08-0f */
	STOP, LD_DE_nn, LD_aDE_A, INC_DE, INC_D, DEC_D, LD_D_n, RLA,	/* 10-17 */
	JR_n, ADD_HL_DE, LD_A_aDE, DEC_DE, INC_E, DEC_E, LD_E_n, RRA,	/* 18-1f */
	JR_NZ_n, LD_HL_nn, LDI_aHL_A, INC_HL, INC_H, DEC_H, LD_H_n, DAA,	/* 20-27 */
	JR_Z_n, ADD_HL_HL, LDI_A_aHL, DEC_HL, INC_L, DEC_L, LD_L_n, CPL,	/* 28-2f */
	JR_NC_n, LD_SP_nn, LDD_aHL_A, INC_SP, INC_aHL, DEC_aHL, LD_aHL_n, SCF,	/* 30-37 */
	JR_C_n, ADD_HL_SP, LDD_A_aHL, DEC_SP, INC_A, DEC_A, LD_A_n, CCF,	/* 38-3f */
	LD_B_B, LD_B_C, LD_B_D, LD_B_E, LD_B_H, LD_B_L, LD_B_aHL, LD_B_A,	/* 40-47 */
	LD_C_B, LD_C_C, LD_C_D, LD_C_E, LD_C_H, LD_C_L, LD_C_aHL, LD_C_A,	/* 48-4f */
	LD_D_B, LD_D_C, LD_D_D, LD_D_E, LD_D_H, LD_D_L, LD_D_aHL, LD_D_A,	/* 50-57 */
	LD_E_B, LD_E_C, LD_E_D, LD_E_E, LD_E_H, LD_E_L, LD_E_aHL, LD_E_A,	/* 50-57 */
	LD_H_B, LD_H_C, LD_H_D, LD_H_E, LD_H_H, LD_H_L, LD_H_aHL, LD_H_A,	/* 60-67 */
	LD_L_B, LD_L_C, LD_L_D, LD_L_E, LD_L_H, LD_L_L, LD_L_aHL, LD_L_A,	/* 68-6f */
	LD_aHL_B, LD_aHL_C, LD_aHL_D, LD_aHL_E, LD_aHL_H, LD_aHL_L, HALT, LD_aHL_A,	/* 70-77 */
	LD_A_B, LD_A_C, LD_A_D, LD_A_E, LD_A_H, LD_A_L, LD_A_aHL, LD_A_A,	/* 78-7f */
	ADD_A_B, ADD_A_C, ADD_A_D, ADD_A_E, ADD_A_H, ADD_A_L, ADD_A_aHL, ADD_A_A,	/* 80-87 */
	ADC_A_B, ADC_A_C, ADC_A_D, ADC_A_E, ADC_A_H, ADC_A_L, ADC_A_aHL, ADC_A_A,	/* 88-8f */
	SUB_B, SUB_C, SUB_D, SUB_E, SUB_H, SUB_L, SUB_aHL, SUB_A,	/* 90-97 */
	SBC_A_B, SBC_A_C, SBC_A_D, SBC_A_E, SBC_A_H, SBC_A_L, SBC_A_aHL, SBC_A_A,	/* 98-9f */
	AND_B, AND_C, AND_D, AND_E, AND_H, AND_L, AND_aHL, AND_A,	/* a0-a7 */
	XOR_B, XOR_C, XOR_D, XOR_E, XOR_H, XOR_L, XOR_aHL, XOR_A,	/* a8-af */
	OR_B, OR_C, OR_D, OR_E, OR_H, OR_L, OR_aHL, OR_A,	/* b0-b7 */
	CP_B, CP_C, CP_D, CP_E, CP_H, CP_L, CP_aHL, CP_A,	/* b8-bf */
	RET_NZ, POP_BC, JP_NZ, JP, CALL_NZ_nn, PUSH_BC, ADD_A_n, RST00,	/* c0-c7 */
	RET_Z, RET, JP_Z, step_cb, CALL_Z_nn, CALL_nn, ADC_A_n, RST08,	/* c8-cf */
	RET_NC, POP_DE, JP_NC, XXX, CALL_NC_nn, PUSH_DE, SUB_n, RST10,	/* d0-d7 */
	RET_C, RETI, JP_C, XXX, CALL_C_nn, XXX, SBC_A_n, RST18,	/* d8-df */
	LDH_an_A, POP_HL, LD_aC_A, XXX, XXX, PUSH_HL, AND_n, RST20,	/* e0-e7 */
	ADD_SP_n, JP_HL, LD_ann_A, XXX, XXX, XXX, XOR_n, RST28,	/* e8-ef */
	LDH_A_an, POP_AF, LD_A_aC, DI, XXX, PUSH_AF, OR_n, RST30,	/* f0-f7 */
	LDHL_SP_n, LD_SP_HL, LD_A_ann, EI, XXX, XXX, CP_n, RST38,	/* f8-ff */
};


static const uint8_t instr_timing[256] = {
	1,3,2,2,1,1,2,1,5,2,2,2,1,1,2,1,
	0,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
	2,3,2,2,1,1,2,1,2,2,2,2,1,1,2,1,
	2,3,2,2,3,3,3,1,2,2,2,2,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	2,2,2,2,2,2,0,2,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	2,3,3,4,3,4,2,4,2,4,3,0,3,6,2,4,
	2,3,3,0,3,4,2,4,2,4,3,0,3,0,2,4,
	3,3,2,0,0,4,2,4,4,1,4,0,0,0,2,4,
	3,3,2,1,0,4,2,4,3,2,4,1,0,0,2,4
};

void step() {
	uint8_t op = rpc8();
	instr_map[op]();
	cycle_counter += instr_timing[op] << 2;
}

void cpu_bios_init() {
	/*
	0x1 - Gameboy/Super Gameboy
	0x11 - Gameboy Color
	0xFF - Gameboy Pocket
	 */
	r.A = 0x1;
	r.F = 0xB0;
	r.BC = 0x13;
	r.DE = 0xD8;
	r.HL = 0x14D;
	r.PC = 0x100;
	r.SP = 0xFFFE;
	
	write(0xFF05, 0x00); // TIMA
	write(0xFF06, 0x00); // TMA
	write(0xFF07, 0x00); // TAC
	write(0xFF10, 0x80); // NR10
	write(0xFF11, 0xBF); // NR11
	write(0xFF12, 0xF3); // NR12
	write(0xFF14, 0xBF); // NR14
	write(0xFF16, 0x3F); // NR21
	write(0xFF17, 0x00); // NR22
	write(0xFF19, 0xBF); // NR24
	write(0xFF1A, 0x7F); // NR30
	write(0xFF1B, 0xFF); // NR31
	write(0xFF1C, 0x9F); // NR32
	write(0xFF1E, 0xBF); // NR33
	write(0xFF20, 0xFF); // NR41
	write(0xFF21, 0x00); // NR42
	write(0xFF22, 0x00); // NR43
	write(0xFF23, 0xBF); // NR30
	write(0xFF24, 0x77); // NR50
	write(0xFF25, 0xF3); // NR51
	write(0xFF26, 0xF1); // NR52 // 0xF1 GB, 0xF0 SGB
	write(0xFF40, 0x91); // LCDC
	write(0xFF42, 0x00); // SCY
	write(0xFF43, 0x00); // SCX
	write(0xFF45, 0x00); // LYC
	write(0xFF47, 0xFC); // BGP
	write(0xFF48, 0xFF); // OBP0
	write(0xFF49, 0xFF); // OBP1
	write(0xFF4A, 0x00); // WY
	write(0xFF4B, 0x00); // WX
	write(0xFFFF, 0x00); // IE
}
