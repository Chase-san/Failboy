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
void LD_A_n(void) { r.A = rpc8(); }
void LD_B_n(void) { r.B = rpc8(); }
void LD_C_n(void) { r.C = rpc8(); }
void LD_D_n(void) { r.D = rpc8(); }
void LD_E_n(void) { r.E = rpc8(); }
void LD_H_n(void) { r.H = rpc8(); }
void LD_L_n(void) { r.L = rpc8(); }
void LD_aHL_n(void) { write(r.HL, rpc8()); }

void LD_A_A(void) { r.A = r.A; }
void LD_A_B(void) { r.A = r.B; }
void LD_A_C(void) { r.A = r.C; }
void LD_A_D(void) { r.A = r.D; }
void LD_A_E(void) { r.A = r.E; }
void LD_A_H(void) { r.A = r.H; }
void LD_A_L(void) { r.A = r.L; }
void LD_A_aHL(void) { r.A = read(r.HL); }

void LD_A_aC(void) { r.A = read(r.C + 0xFF00); }
void LD_aC_A(void) { write(r.C + 0xFF00, r.A); }

void LD_A_aBC(void) { r.A = read(r.BC); }
void LD_A_aDE(void) { r.A = read(r.DE); }
void LD_A_ann(void) { r.A = read(rpc16()); }

void LD_B_A(void) { r.B = r.A; }
void LD_B_B(void) { r.B = r.B; }
void LD_B_C(void) { r.B = r.C; }
void LD_B_D(void) { r.B = r.D; }
void LD_B_E(void) { r.B = r.E; }
void LD_B_H(void) { r.B = r.H; }
void LD_B_L(void) { r.B = r.L; }
void LD_B_aHL(void) { r.B = read(r.HL); }

void LD_C_A(void) { r.C = r.A; }
void LD_C_B(void) { r.C = r.B; }
void LD_C_C(void) { r.C = r.C; }
void LD_C_D(void) { r.C = r.D; }
void LD_C_E(void) { r.C = r.E; }
void LD_C_H(void) { r.C = r.H; }
void LD_C_L(void) { r.C = r.L; }
void LD_C_aHL(void) { r.C = read(r.HL); }

void LD_D_A(void) { r.D = r.A; }
void LD_D_B(void) { r.D = r.B; }
void LD_D_C(void) { r.D = r.C; }
void LD_D_D(void) { r.D = r.D; }
void LD_D_E(void) { r.D = r.E; }
void LD_D_H(void) { r.D = r.H; }
void LD_D_L(void) { r.D = r.L; }
void LD_D_aHL(void) { r.D = read(r.HL); }

void LD_E_A(void) { r.E = r.A; }
void LD_E_B(void) { r.E = r.B; }
void LD_E_C(void) { r.E = r.C; }
void LD_E_D(void) { r.E = r.D; }
void LD_E_E(void) { r.E = r.E; }
void LD_E_H(void) { r.E = r.H; }
void LD_E_L(void) { r.E = r.L; }
void LD_E_aHL(void) { r.E = read(r.HL); }

void LD_H_A(void) { r.H = r.A; }
void LD_H_B(void) { r.H = r.B; }
void LD_H_C(void) { r.H = r.C; }
void LD_H_D(void) { r.H = r.D; }
void LD_H_E(void) { r.H = r.E; }
void LD_H_H(void) { r.H = r.H; }
void LD_H_L(void) { r.H = r.L; }
void LD_H_aHL(void) { r.H = read(r.HL); }

void LD_L_A(void) { r.L = r.A; }
void LD_L_B(void) { r.L = r.B; }
void LD_L_C(void) { r.L = r.C; }
void LD_L_D(void) { r.L = r.D; }
void LD_L_E(void) { r.L = r.E; }
void LD_L_H(void) { r.L = r.H; }
void LD_L_L(void) { r.L = r.L; }
void LD_L_aHL(void) { r.L = read(r.HL); }

void LD_aHL_A(void) { write(r.HL, r.A); }
void LD_aHL_B(void) { write(r.HL, r.B); }
void LD_aHL_C(void) { write(r.HL, r.C); }
void LD_aHL_D(void) { write(r.HL, r.D); }
void LD_aHL_E(void) { write(r.HL, r.E); }
void LD_aHL_H(void) { write(r.HL, r.H); }
void LD_aHL_L(void) { write(r.HL, r.L); }

void LD_aBC_A(void) { write(r.BC, r.A); }
void LD_aDE_A(void) { write(r.DE, r.A); }
void LD_ann_A(void) { write(rpc16(), r.A); }

void LDD_A_aHL(void) { r.A = read(r.HL--); }
void LDD_aHL_A(void) { write(r.HL--, r.A); }

void LDI_A_aHL(void) { r.A = read(r.HL++); }
void LDI_aHL_A(void) { write(r.HL++, r.A); }

void LDH_A_an(void) { r.A = read(0xFF00 + rpc8()); }
void LDH_an_A(void) { write(0xFF00 + rpc8(), r.A); }


/* **************************************** */
/* 16-bit loads */

void LD_BC_nn(void) { r.BC = rpc16(); }
void LD_DE_nn(void) { r.DE = rpc16(); }
void LD_HL_nn(void) { r.HL = rpc16(); }
void LD_SP_nn(void) { r.SP = rpc16(); }

void LD_SP_HL(void) { r.SP = r.HL; }

void LDHL_SP_n(void) {
	register int8_t n = rpc8();
	r.F = 0;
	r.F_H = (r.SP & 0xF) + (n & 0xf) > 0xF;
	r.F_C = (r.SP & 0xFF) + (n & 0xfF) > 0xFF;
	r.HL = (uint16_t)(r.SP + n);
}

void LD_ann_SP(void) { write16(rpc16(), r.SP); }

static inline void PUSH16(uint16_t n) {
	r.SP -= 2;
	write16(r.SP, n);
}
void PUSH_AF(void) { PUSH16(r.AF); }
void PUSH_BC(void) { PUSH16(r.BC); }
void PUSH_DE(void) { PUSH16(r.DE); }
void PUSH_HL(void) { PUSH16(r.HL); }

static inline uint16_t POP16(void) {
	uint16_t ret = read16(r.SP);
	r.SP += 2;
	return ret;
}

/* Lower bits of F are never ever set. */
void POP_AF(void) { r.AF = POP16() & 0xFFF0; }
void POP_BC(void) { r.BC = POP16(); }
void POP_DE(void) { r.DE = POP16(); }
void POP_HL(void) { r.HL = POP16(); }

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
void ADD_A_A(void) { ADD(r.A); }
void ADD_A_B(void) { ADD(r.B); }
void ADD_A_C(void) { ADD(r.C); }
void ADD_A_D(void) { ADD(r.D); }
void ADD_A_E(void) { ADD(r.E); }
void ADD_A_H(void) { ADD(r.H); }
void ADD_A_L(void) { ADD(r.L); }
void ADD_A_aHL(void) { ADD(read(r.HL)); }
void ADD_A_n(void) { ADD(rpc8()); }

void ADC_A_A(void) { ADD(r.A + r.F_C); }
void ADC_A_B(void) { ADD(r.B + r.F_C); }
void ADC_A_C(void) { ADD(r.C + r.F_C); }
void ADC_A_D(void) { ADD(r.D + r.F_C); }
void ADC_A_E(void) { ADD(r.E + r.F_C); }
void ADC_A_H(void) { ADD(r.H + r.F_C); }
void ADC_A_L(void) { ADD(r.L + r.F_C); }
void ADC_A_aHL(void) { ADD(read(r.HL) + r.F_C); }
void ADC_A_n(void) { ADD(rpc8() + r.F_C); }

static inline void SUB(uint8_t n) {
	r.F = 0;
	r.F_N = 1;
	r.F_H = (r.A & 0xF) < (n & 0xF);
	r.F_C = r.A < n;
	r.A -= n;
	r.F_Z = !r.A;
}

void SUB_A(void) { SUB(r.A); }
void SUB_B(void) { SUB(r.B); }
void SUB_C(void) { SUB(r.C); }
void SUB_D(void) { SUB(r.D); }
void SUB_E(void) { SUB(r.E); }
void SUB_H(void) { SUB(r.H); }
void SUB_L(void) { SUB(r.L); }
void SUB_aHL(void) { SUB(read(r.HL)); }
void SUB_n(void) { SUB(rpc8()); }

void SBC_A_A(void) { SUB(r.A + r.F_C); }
void SBC_A_B(void) { SUB(r.B + r.F_C); }
void SBC_A_C(void) { SUB(r.C + r.F_C); }
void SBC_A_D(void) { SUB(r.D + r.F_C); }
void SBC_A_E(void) { SUB(r.E + r.F_C); }
void SBC_A_H(void) { SUB(r.H + r.F_C); }
void SBC_A_L(void) { SUB(r.L + r.F_C); }
void SBC_A_aHL(void) { SUB(read(r.HL) + r.F_C); }
void SBC_A_n(void) { SUB(rpc8() + r.F_C); }

static inline void AND(uint8_t n) {
	r.A &= n;
	r.F = 0;
	r.F_H = 1;
	r.F_Z = !r.A;
}
void AND_A(void) { AND(r.A); }
void AND_B(void) { AND(r.B); }
void AND_C(void) { AND(r.C); }
void AND_D(void) { AND(r.D); }
void AND_E(void) { AND(r.E); }
void AND_H(void) { AND(r.H); }
void AND_L(void) { AND(r.L); }
void AND_aHL(void) { AND(read(r.HL)); }
void AND_n(void) { AND(rpc8()); }

static inline void OR(uint8_t n) {
	r.A |= n;
	r.F = 0;
	r.F_Z = !r.A;
}
void OR_A(void) { OR(r.A); }
void OR_B(void) { OR(r.B); }
void OR_C(void) { OR(r.C); }
void OR_D(void) { OR(r.D); }
void OR_E(void) { OR(r.E); }
void OR_H(void) { OR(r.H); }
void OR_L(void) { OR(r.L); }
void OR_aHL(void) { OR(read(r.HL)); }
void OR_n(void) { OR(rpc8()); }

static inline void XOR(uint8_t n) {
	r.A |= n;
	r.F = 0;
	r.F_Z = !r.A;
}
void XOR_A(void) { XOR(r.A); }
void XOR_B(void) { XOR(r.B); }
void XOR_C(void) { XOR(r.C); }
void XOR_D(void) { XOR(r.D); }
void XOR_E(void) { XOR(r.E); }
void XOR_H(void) { XOR(r.H); }
void XOR_L(void) { XOR(r.L); }
void XOR_aHL(void) { XOR(read(r.HL)); }
void XOR_n(void) { XOR(rpc8()); }

static inline void CP(uint8_t n) {
	r.F = 0;
	r.F_N = 1;
	r.F_H = (r.A & 0xF) < (n & 0xF);
	r.F_C = r.A < n;
	r.F_Z = r.A == n;
}
void CP_A(void) { CP(r.A); }
void CP_B(void) { CP(r.B); }
void CP_C(void) { CP(r.C); }
void CP_D(void) { CP(r.D); }
void CP_E(void) { CP(r.E); }
void CP_H(void) { CP(r.H); }
void CP_L(void) { CP(r.L); }
void CP_aHL(void) { CP(read(r.HL)); }
void CP_n(void) { CP(rpc8()); }

static inline void INC(uint8_t n) {
	r.F_N = 0;
	r.F_H = !(n & 0xf);
	r.F_Z = !n;
}
void INC_A(void) { INC(++r.A); }
void INC_B(void) { INC(++r.B); }
void INC_C(void) { INC(++r.C); }
void INC_D(void) { INC(++r.D); }
void INC_E(void) { INC(++r.E); }
void INC_H(void) { INC(++r.H); }
void INC_L(void) { INC(++r.L); }
void INC_aHL(void) {
	register uint8_t tmp = read(r.HL) + 1;
	write(r.HL, tmp);
	INC(tmp);
}

static inline void DEC(uint8_t n) {
	r.F_N = 1;
	r.F_H = (n & 0xf) == 0xf;
	r.F_Z = !n;
}
void DEC_A(void) { DEC(--r.A); }
void DEC_B(void) { DEC(--r.B); }
void DEC_C(void) { DEC(--r.C); }
void DEC_D(void) { DEC(--r.D); }
void DEC_E(void) { DEC(--r.E); }
void DEC_H(void) { DEC(--r.H); }
void DEC_L(void) { DEC(--r.L); }
void DEC_aHL(void) {
	register uint8_t tmp = read(r.HL) - 1;
	write(r.HL, tmp);
	DEC(tmp);
}


/* **************************************** */
/* 16-bit Arithmetic (ALU16) */
static inline void ADD_HL(uint16_t n) {
	r.F_N = 0;
	r.F_H = (r.HL & 0xFFF) + (n & 0xFFF) > 0xFFF;
	r.F_C = (r.HL + n) > 0xFFFF;
	r.HL = r.HL + n;
}
void ADD_HL_BC(void) { ADD_HL(r.BC); }
void ADD_HL_DE(void) { ADD_HL(r.DE); }
void ADD_HL_HL(void) { ADD_HL(r.HL); }
void ADD_HL_SP(void) { ADD_HL(r.SP); }

void ADD_SP_n(void) {
	register int8_t n = rpc8();
	r.F = 0;
	r.F_H = (r.SP & 0xF) + (n & 0xf) > 0xF;
	r.F_C = (r.SP & 0xFF) + (n & 0xfF) > 0xFF;
	r.SP = (uint16_t)(r.SP + n);
}

void INC_BC(void) { r.BC += 1; }
void INC_DE(void) { r.DE += 1; }
void INC_HL(void) { r.HL += 1; }
void INC_SP(void) { r.SP += 1; }

void DEC_BC(void) { r.BC -= 1; }
void DEC_DE(void) { r.DE -= 1; }
void DEC_HL(void) { r.HL -= 1; }
void DEC_SP(void) { r.SP -= 1; }

/* **************************************** */
/* Rotates & Shifts */
void RLCA(void) {
	register uint8_t bit = (r.A >> 7) & 1;
	r.A = (r.A << 1) | bit;
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !r.A;
}
void RLA(void) {
	register uint8_t bit = (r.A >> 7) & 1;
	r.A = (r.A << 1) | r.F_C;
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !r.A;
}
void RRCA(void) {
	register uint8_t bit = r.A & 1;
	r.A = (r.A >> 1) | (bit << 7);
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !r.A;
}
void RRA(void) {
	register uint8_t bit = r.A & 1;
	r.A = (r.A >> 1) | (r.F_C << 7);
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !r.A;
}


/* **************************************** */
/* Jumps */
void JP(void) { r.PC = rpc16(); }
void JP_NZ(void) {
	register uint16_t addr = rpc16();
	if(!r.F_Z) {
		r.PC = addr;
	}
}
void JP_Z(void) {
	register uint16_t addr = rpc16();
	if(r.F_Z) {
		r.PC = addr;
	}
}
void JP_NC(void) {
	register uint16_t addr = rpc16();
	if(!r.F_C) {
		r.PC = addr;
	}
}
void JP_C(void) {
	register uint16_t addr = rpc16();
	if(r.F_C) {
		r.PC = addr;
	}
}
void JP_HL(void) { r.PC = r.HL; }

static inline void JR(int8_t n) {
	r.PC += n;
}

void JR_n(void) { JR(rpc8()); }

void JR_NZ_n(void) {
	register uint8_t n = rpc8();
	if(!r.F_Z) {
		JR(n);
	}
}
void JR_Z_n(void) {
	register uint8_t n = rpc8();
	if(r.F_Z) {
		JR(n);
	}
}
void JR_NC_n(void) {
	register uint8_t n = rpc8();
	if(!r.F_C) {
		JR(n);
	}
}
void JR_C_n(void) {
	register uint8_t n = rpc8();
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

void CALL_nn(void) { CALL(rpc16()); }

void CALL_NZ_nn(void) {
	register uint16_t addr = rpc16();
	if(!r.F_Z) {
		CALL(addr);
	}
}
void CALL_Z_nn(void) {
	register uint16_t addr = rpc16();
	if(r.F_Z) {
		CALL(addr);
	}
}
void CALL_NC_nn(void) {
	register uint16_t addr = rpc16();
	if(!r.F_C) {
		CALL(addr);
	}
}
void CALL_C_nn(void) {
	register uint16_t addr = rpc16();
	if(r.F_C) {
		CALL(addr);
	}
}

/* **************************************** */
/* Restarts */
void RST00(void) { r.PC = 0x00; }
void RST08(void) { r.PC = 0x08; }
void RST10(void) { r.PC = 0x10; }
void RST18(void) { r.PC = 0x18; }
void RST20(void) { r.PC = 0x20; }
void RST28(void) { r.PC = 0x28; }
void RST30(void) { r.PC = 0x30; }
void RST38(void) { r.PC = 0x38; }

/* **************************************** */
/* Misc */
void CPL(void) {
	r.F_N = 1;
	r.F_H = 1;
	r.A ^= 0xFF;
}
void CCF(void) {
	r.F_N = r.F_H = 0;
	r.F_C ^= 1;
}
void SCF(void) {
	r.F_N = r.F_H = 0;
	r.F_C = 1;
}
void DAA(void) {
	/* //sigh// let's get this shit over with */
	register int32_t tmp = r.A;
	
	if(r.F_N) {
		if(r.F_H) {
			tmp -= 6;
			if(!r.F_C) {
				tmp &= 0xFF;
			}
		}
		if(r.F_C) {
			tmp -= 0x60;
		}
	} else {
		if(r.F_H || (tmp & 0xF) > 0x9) {
			tmp += 0x6;
		}
		if(r.F_C || tmp > 0x9F) {
			tmp += 0x60;
		}
	}
	
	r.F_H = 0;
	if(tmp & 0x100)
		r.F_C = 1;
	r.A = tmp & 0xFF;
	r.F_Z = !r.A;
}
void HALT(void) { /* DO HALT */ }
void STOP(void) { }

void DI(void) { }
void EI(void) { }

/* **************************************** */
/* Return */
void RET(void) { r.PC = POP16(); }

void RET_NZ(void) {
	if(!r.F_Z) {
		RET();
	}
}
void RET_Z(void) {
	if(r.F_Z) {
		RET();
	}
}
void RET_NC(void) {
	if(!r.F_C) {
		RET();
	}
}
void RET_C(void) {
	if(r.F_C) {
		RET();
	}
}

void RETI(void) { RET(); EI(); }
