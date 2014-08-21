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

/* **************************************** */
/* 8-bit loads */
void LD_A_n() { r.A = rpc8(); }
void LD_B_n() { r.B = rpc8(); }
void LD_C_n() { r.C = rpc8(); }
void LD_D_n() { r.D = rpc8(); }
void LD_E_n() { r.E = rpc8(); }
void LD_H_n() { r.H = rpc8(); }
void LD_L_n() { r.L = rpc8(); }
void LD_aHL_n() { write(r.HL, rpc8()); }

void LD_A_A() { r.A = r.A; }
void LD_A_B() { r.A = r.B; }
void LD_A_C() { r.A = r.C; }
void LD_A_D() { r.A = r.D; }
void LD_A_E() { r.A = r.E; }
void LD_A_H() { r.A = r.H; }
void LD_A_L() { r.A = r.L; }
void LD_A_aHL() { r.A = read(r.HL); }

void LD_A_aC() { r.A = read(r.C + 0xFF00); }
void LD_aC_A() { write(r.C + 0xFF00, r.A); }

void LD_A_aBC() { r.A = read(r.BC); }
void LD_A_aDE() { r.A = read(r.DE); }
void LD_A_ann() { r.A = read(rpc16()); }

void LD_B_A() { r.B = r.A; }
void LD_B_B() { r.B = r.B; }
void LD_B_C() { r.B = r.C; }
void LD_B_D() { r.B = r.D; }
void LD_B_E() { r.B = r.E; }
void LD_B_H() { r.B = r.H; }
void LD_B_L() { r.B = r.L; }
void LD_B_aHL() { r.B = read(r.HL); }

void LD_C_A() { r.C = r.A; }
void LD_C_B() { r.C = r.B; }
void LD_C_C() { r.C = r.C; }
void LD_C_D() { r.C = r.D; }
void LD_C_E() { r.C = r.E; }
void LD_C_H() { r.C = r.H; }
void LD_C_L() { r.C = r.L; }
void LD_C_aHL() { r.C = read(r.HL); }

void LD_D_A() { r.D = r.A; }
void LD_D_B() { r.D = r.B; }
void LD_D_C() { r.D = r.C; }
void LD_D_D() { r.D = r.D; }
void LD_D_E() { r.D = r.E; }
void LD_D_H() { r.D = r.H; }
void LD_D_L() { r.D = r.L; }
void LD_D_aHL() { r.D = read(r.HL); }

void LD_E_A() { r.E = r.A; }
void LD_E_B() { r.E = r.B; }
void LD_E_C() { r.E = r.C; }
void LD_E_D() { r.E = r.D; }
void LD_E_E() { r.E = r.E; }
void LD_E_H() { r.E = r.H; }
void LD_E_L() { r.E = r.L; }
void LD_E_aHL() { r.E = read(r.HL); }

void LD_H_A() { r.H = r.A; }
void LD_H_B() { r.H = r.B; }
void LD_H_C() { r.H = r.C; }
void LD_H_D() { r.H = r.D; }
void LD_H_E() { r.H = r.E; }
void LD_H_H() { r.H = r.H; }
void LD_H_L() { r.H = r.L; }
void LD_H_aHL() { r.H = read(r.HL); }

void LD_L_A() { r.L = r.A; }
void LD_L_B() { r.L = r.B; }
void LD_L_C() { r.L = r.C; }
void LD_L_D() { r.L = r.D; }
void LD_L_E() { r.L = r.E; }
void LD_L_H() { r.L = r.H; }
void LD_L_L() { r.L = r.L; }
void LD_L_aHL() { r.L = read(r.HL); }

void LD_aHL_A() { write(r.HL, r.A); }
void LD_aHL_B() { write(r.HL, r.B); }
void LD_aHL_C() { write(r.HL, r.C); }
void LD_aHL_D() { write(r.HL, r.D); }
void LD_aHL_E() { write(r.HL, r.E); }
void LD_aHL_H() { write(r.HL, r.H); }
void LD_aHL_L() { write(r.HL, r.L); }

void LD_aBC_A() { write(r.BC, r.A); }
void LD_aDE_A() { write(r.DE, r.A); }
void LD_ann_A() { write(rpc16(), r.A); }

void LDD_A_aHL() { r.A = read(r.HL--); }
void LDD_aHL_A() { write(r.HL--, r.A); }

void LDI_A_aHL() { r.A = read(r.HL++); }
void LDI_aHL_A() { write(r.HL++, r.A); }

void LDH_A_an() { r.A = read(0xFF00 + rpc8()); }
void LDH_an_A() { write(0xFF00 + rpc8(), r.A); }


/* **************************************** */
/* 16-bit loads */

void LD_BC_nn() { r.BC = rpc16(); }
void LD_DE_nn() { r.DE = rpc16(); }
void LD_HL_nn() { r.HL = rpc16(); }
void LD_SP_nn() { r.SP = rpc16(); }

void LD_SP_HL() { r.SP = r.HL; }

void LDHL_SP_n() {
	char n = rpc8();
	r.F = 0;
	r.F_H = (r.SP & 0xF) + (n & 0xf) > 0xF;
	r.F_C = (r.SP & 0xFF) + (n & 0xfF) > 0xFF;
	r.HL = r.SP + n;
}

void LD_ann_SP() { write16(rpc16(), r.SP); }

static inline void PUSH16(uint16_t n) {
	r.SP -= 2;
	write16(r.SP, n);
}
void PUSH_AF() { PUSH16(r.AF); }
void PUSH_BC() { PUSH16(r.BC); }
void PUSH_DE() { PUSH16(r.DE); }
void PUSH_HL() { PUSH16(r.HL); }

static inline uint16_t POP16() {
	uint16_t ret = read16(r.SP);
	r.SP += 2;
	return ret;
}

void POP_AF() { r.AF = POP16(); }
void POP_BC() { r.BC = POP16(); }
void POP_DE() { r.DE = POP16(); }
void POP_HL() { r.HL = POP16(); }

/* **************************************** */
/* 8-bit Arithmetic (ALU8) */

static inline void ADD(uint8_t n) {
	r.F = 0;
	r.F_N = 0;
	r.F_H = (r.A & 0xF) + (n & 0xF) > 0xF;
	r.F_C = (r.A + n) > 0xFF;
	r.A += n;
	r.F_Z = !r.A;
}
void ADD_A_A() { ADD(r.A); }
void ADD_A_B() { ADD(r.B); }
void ADD_A_C() { ADD(r.C); }
void ADD_A_D() { ADD(r.D); }
void ADD_A_E() { ADD(r.E); }
void ADD_A_H() { ADD(r.H); }
void ADD_A_L() { ADD(r.L); }
void ADD_A_aHL() { ADD(read(r.HL)); }
void ADD_A_n() { ADD(rpc8()); }

void ADC_A_A() { ADD(r.A + r.F_C); }
void ADC_A_B() { ADD(r.B + r.F_C); }
void ADC_A_C() { ADD(r.C + r.F_C); }
void ADC_A_D() { ADD(r.D + r.F_C); }
void ADC_A_E() { ADD(r.E + r.F_C); }
void ADC_A_H() { ADD(r.H + r.F_C); }
void ADC_A_L() { ADD(r.L + r.F_C); }
void ADC_A_aHL() { ADD(read(r.HL) + r.F_C); }
void ADC_A_n() { ADD(rpc8() + r.F_C); }

static inline void SUB(uint8_t n) {
	r.F = 0;
	r.F_N = 1;
	r.F_H = (r.A & 0xF) < (n & 0xF);
	r.F_C = r.A < n;
	r.A -= n;
	r.F_Z = !r.A;
}

void SUB_A() { SUB(r.A); }
void SUB_B() { SUB(r.B); }
void SUB_C() { SUB(r.C); }
void SUB_D() { SUB(r.D); }
void SUB_E() { SUB(r.E); }
void SUB_H() { SUB(r.H); }
void SUB_L() { SUB(r.L); }
void SUB_aHL() { SUB(read(r.HL)); }
void SUB_n() { SUB(rpc8()); }

void SBC_A_A() { SUB(r.A + r.F_C); }
void SBC_A_B() { SUB(r.B + r.F_C); }
void SBC_A_C() { SUB(r.C + r.F_C); }
void SBC_A_D() { SUB(r.D + r.F_C); }
void SBC_A_E() { SUB(r.E + r.F_C); }
void SBC_A_H() { SUB(r.H + r.F_C); }
void SBC_A_L() { SUB(r.L + r.F_C); }
void SBC_A_aHL() { SUB(read(r.HL) + r.F_C); }
void SBC_A_n() { SUB(rpc8() + r.F_C); }

static inline void AND(uint8_t n) {
	r.A &= n;
	r.F = 0;
	r.F_H = 1;
	r.F_Z = !r.A;
}
void AND_A() { AND(r.A); }
void AND_B() { AND(r.B); }
void AND_C() { AND(r.C); }
void AND_D() { AND(r.D); }
void AND_E() { AND(r.E); }
void AND_H() { AND(r.H); }
void AND_L() { AND(r.L); }
void AND_aHL() { AND(read(r.HL)); }
void AND_n() { AND(rpc8()); }

static inline void OR(uint8_t n) {
	r.A |= n;
	r.F = 0;
	r.F_Z = !r.A;
}
void OR_A() { OR(r.A); }
void OR_B() { OR(r.B); }
void OR_C() { OR(r.C); }
void OR_D() { OR(r.D); }
void OR_E() { OR(r.E); }
void OR_H() { OR(r.H); }
void OR_L() { OR(r.L); }
void OR_aHL() { OR(read(r.HL)); }
void OR_n() { OR(rpc8()); }

static inline void XOR(uint8_t n) {
	r.A |= n;
	r.F = 0;
	r.F_Z = !r.A;
}
void XOR_A() { XOR(r.A); }
void XOR_B() { XOR(r.B); }
void XOR_C() { XOR(r.C); }
void XOR_D() { XOR(r.D); }
void XOR_E() { XOR(r.E); }
void XOR_H() { XOR(r.H); }
void XOR_L() { XOR(r.L); }
void XOR_aHL() { XOR(read(r.HL)); }
void XOR_n() { XOR(rpc8()); }

static inline void CP(uint8_t n) {
	r.F = 0;
	r.F_N = 1;
	r.F_H = (r.A & 0xF) < (n & 0xF);
	r.F_C = r.A < n;
	r.F_Z = r.A == n;
}
void CP_A() { CP(r.A); }
void CP_B() { CP(r.B); }
void CP_C() { CP(r.C); }
void CP_D() { CP(r.D); }
void CP_E() { CP(r.E); }
void CP_H() { CP(r.H); }
void CP_L() { CP(r.L); }
void CP_aHL() { CP(read(r.HL)); }
void CP_n() { CP(rpc8()); }

static inline void INC(uint8_t n) {
	r.F_N = 0;
	r.F_H = !(n & 0xf);
	r.F_Z = !n;
}
void INC_A() { INC(++r.A); }
void INC_B() { INC(++r.B); }
void INC_C() { INC(++r.C); }
void INC_D() { INC(++r.D); }
void INC_E() { INC(++r.E); }
void INC_H() { INC(++r.H); }
void INC_L() { INC(++r.L); }
void INC_aHL() {
	uint8_t tmp = read(r.HL) + 1;
	write(r.HL, tmp);
	INC(tmp);
}

static inline void DEC(uint8_t n) {
	r.F_N = 1;
	r.F_H = (n & 0xf) == 0xf;
	r.F_Z = !n;
}
void DEC_A() { DEC(--r.A); }
void DEC_B() { DEC(--r.B); }
void DEC_C() { DEC(--r.C); }
void DEC_D() { DEC(--r.D); }
void DEC_E() { DEC(--r.E); }
void DEC_H() { DEC(--r.H); }
void DEC_L() { DEC(--r.L); }
void DEC_aHL() {
	uint8_t tmp = read(r.HL) - 1;
	write(r.HL, tmp);
	DEC(tmp);
}


/* **************************************** */
/* 16-bit Arithmetic (ALU16) */
static inline void ADD_HL(uint16_t n) {
	r.F_N = 0;
	r.F_H = (r.HL & 0xFFF) + (n & 0xFFF) > 0xFFF;
	r.F_C = (r.HL + n) > 0xFFFF;
	r.HL += n;
}
void ADD_HL_BC() { ADD_HL(r.BC); }
void ADD_HL_DE() { ADD_HL(r.DE); }
void ADD_HL_HL() { ADD_HL(r.HL); }
void ADD_HL_SP() { ADD_HL(r.SP); }

void ADD_SP_n() {
	char n = rpc8();
	r.F = 0;
	r.F_H = (r.SP & 0xF) + (n & 0xf) > 0xF;
	r.F_C = (r.SP & 0xFF) + (n & 0xfF) > 0xFF;
	r.SP += n;
}

void INC_BC() { ++r.BC; }
void INC_DE() { ++r.DE; }
void INC_HL() { ++r.HL; }
void INC_SP() { ++r.SP; }

void DEC_BC() { --r.BC; }
void DEC_DE() { --r.DE; }
void DEC_HL() { --r.HL; }
void DEC_SP() { --r.SP; }

/* **************************************** */
/* Rotates & Shifts */
void RLCA() {
	uint8_t bit = (r.A >> 7) & 1;
	r.A = (r.A << 1) | bit;
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !r.A;
}
void RLA() {
	uint8_t bit = (r.A >> 7) & 1;
	r.A = (r.A << 1) | r.F_C;
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !r.A;
}
void RRCA() {
	uint8_t bit = r.A & 1;
	r.A = (r.A >> 1) | (bit << 7);
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !r.A;
}
void RRA() {
	uint8_t bit = r.A & 1;
	r.A = (r.A >> 1) | (r.F_C << 7);
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !r.A;
}


/* **************************************** */
/* Jumps */
void JP() { r.PC = rpc16(); }
void JP_NZ() {
	uint16_t addr = rpc16();
	if(!r.F_Z) {
		r.PC = addr;
	}
}
void JP_Z() {
	uint16_t addr = rpc16();
	if(r.F_Z) {
		r.PC = addr;
	}
}
void JP_NC() {
	uint16_t addr = rpc16();
	if(!r.F_C) {
		r.PC = addr;
	}
}
void JP_C() {
	uint16_t addr = rpc16();
	if(r.F_C) {
		r.PC = addr;
	}
}
void JP_HL() { r.PC = r.HL; }

static inline void JR(int8_t n) {
	r.PC += n;
}

void JR_n() { JR(rpc8()); }

void JR_NZ_n() {
	uint8_t n = rpc8();
	if(!r.F_Z) {
		JR(n);
	}
}
void JR_Z_n() {
	uint8_t n = rpc8();
	if(r.F_Z) {
		JR(n);
	}
}
void JR_NC_n() {
	uint8_t n = rpc8();
	if(!r.F_C) {
		JR(n);
	}
}
void JR_C_n() {
	uint8_t n = rpc8();
	if(r.F_C) {
		JR(n);
	}
}

/* **************************************** */
/* Calls */
static inline void CALL(uint16_t address) {
	PUSH16(r.PC);
	r.PC = address;
}

void CALL_nn() { CALL(rpc16()); }

void CALL_NZ_nn() {
	uint16_t addr = rpc16();
	if(!r.F_Z) {
		CALL(addr);
	}
}
void CALL_Z_nn() {
	uint16_t addr = rpc16();
	if(r.F_Z) {
		CALL(addr);
	}
}
void CALL_NC_nn() {
	uint16_t addr = rpc16();
	if(!r.F_C) {
		CALL(addr);
	}
}
void CALL_C_nn() {
	uint16_t addr = rpc16();
	if(r.F_C) {
		CALL(addr);
	}
}

/* **************************************** */
/* Restarts */
void RST00() { r.PC = 0x00; }
void RST08() { r.PC = 0x08; }
void RST10() { r.PC = 0x10; }
void RST18() { r.PC = 0x18; }
void RST20() { r.PC = 0x20; }
void RST28() { r.PC = 0x28; }
void RST30() { r.PC = 0x30; }
void RST38() { r.PC = 0x38; }

/* **************************************** */
/* Misc */
void CPL() {
	r.F_N = 1;
	r.F_H = 1;
	r.A ^= 0xFF;
}
void CCF() {
	r.F_N = r.F_H = 0;
	r.F_C ^= 1;
}
void SCF() {
	r.F_N = r.F_H = 0;
	r.F_C = 1;
}
void DAA() {
	/* sigh let's get this shit over with */
	/* Every time someone has to emulate DAA, some cute anmial perishes */
	uint16_t tmp = r.A;
	if(r.F_N) {
		if(r.F_H) {
			tmp -= 6;
		}
		if(r.F_C) {
			tmp -= 0x60;
		}
	} else {
		if(r.F_H || (r.A & 0xF) > 9) {
			tmp += 6;
		}
		if(r.F_C || r.A > 0x9F) {
			tmp += 0x60;
		}
	}
	r.F_H = 0;
	r.F_C = (tmp & 0x100) == 0x100;
	r.A = tmp & 0xFF;
	r.F_Z = !r.A;
}
void HALT() { /* DO HALT */ }
void STOP() { }

void DI() { }
void EI() { }

/* **************************************** */
/* Return */
void RET() { r.PC = POP16(); }

void RET_NZ() {
	if(!r.F_Z) {
		RET();
	}
}
void RET_Z() {
	if(r.F_Z) {
		RET();
	}
}
void RET_NC() {
	if(!r.F_C) {
		RET();
	}
}
void RET_C() {
	if(r.F_C) {
		RET();
	}
}

void RETI() { RET(); EI(); }
