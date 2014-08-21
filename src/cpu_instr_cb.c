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

/* CB Instructions */
static inline uint8_t SWAP(uint8_t n) {
	r.F = 0;
	r.F_Z = n == 0;
	return (n >> 4) | (n << 4);
}
void SWAP_A() { r.A = SWAP(r.A); }
void SWAP_B() { r.B = SWAP(r.B); }
void SWAP_C() { r.C = SWAP(r.C); }
void SWAP_D() { r.D = SWAP(r.D); }
void SWAP_E() { r.E = SWAP(r.E); }
void SWAP_H() { r.H = SWAP(r.H); }
void SWAP_L() { r.L = SWAP(r.L); }
void SWAP_aHL() { write(r.HL, SWAP(read(r.HL))); }

static inline uint8_t RLC_n(register uint8_t n) {
	register uint8_t bit = (n >> 7) & 1;
	n = (n << 1) | bit;
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !n;
	return n;
}

void RLC_B() { r.B = RLC_n(r.B); }
void RLC_C() { r.C = RLC_n(r.C); }
void RLC_D() { r.D = RLC_n(r.D); }
void RLC_E() { r.E = RLC_n(r.E); }
void RLC_H() { r.H = RLC_n(r.H); }
void RLC_L() { r.L = RLC_n(r.L); }
void RLC_aHL() { write(r.HL, RLC_n(read(r.HL))); }

static inline uint8_t RL_n(register uint8_t n) {
	register uint8_t bit = (n >> 7) & 1;
	n = (n << 1) | r.F_C;
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !n;
	return n;
}

void RL_B() { r.B = RL_n(r.B); }
void RL_C() { r.C = RL_n(r.C); }
void RL_D() { r.D = RL_n(r.D); }
void RL_E() { r.E = RL_n(r.E); }
void RL_H() { r.H = RL_n(r.H); }
void RL_L() { r.L = RL_n(r.L); }
void RL_aHL() { write(r.HL, RL_n(read(r.HL))); }

static inline uint8_t RRC_n(register uint8_t n) {
	register uint8_t bit = n & 1;
	n = (n >> 1) | (bit << 7);
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !n;
	return n;
}

void RRC_B() { r.B = RRC_n(r.B); }
void RRC_C() { r.C = RRC_n(r.C); }
void RRC_D() { r.D = RRC_n(r.D); }
void RRC_E() { r.E = RRC_n(r.E); }
void RRC_H() { r.H = RRC_n(r.H); }
void RRC_L() { r.L = RRC_n(r.L); }
void RRC_aHL() { write(r.HL, RRC_n(read(r.HL))); }

static inline uint8_t RR_n(register uint8_t n) {
	register uint8_t bit = n & 1;
	n = (n >> 1) | (r.F_C << 7);
	r.F = 0;
	r.F_C = bit;
	r.F_Z = !n;
	return n;
}

void RR_B() { r.B = RR_n(r.B); }
void RR_C() { r.C = RR_n(r.C); }
void RR_D() { r.D = RR_n(r.D); }
void RR_E() { r.E = RR_n(r.E); }
void RR_H() { r.H = RR_n(r.H); }
void RR_L() { r.L = RR_n(r.L); }
void RR_aHL() { write(r.HL, RR_n(read(r.HL))); }

static inline uint8_t SLA_n(register uint8_t n) {
	r.F_C = (n >> 7) & 1;
	n <<= 1;
	r.F_Z = !n;
	return n;
}

void SLA_A() { r.A = SLA_n(r.A); }
void SLA_B() { r.B = SLA_n(r.B); }
void SLA_C() { r.C = SLA_n(r.C); }
void SLA_D() { r.D = SLA_n(r.D); }
void SLA_E() { r.E = SLA_n(r.E); }
void SLA_H() { r.H = SLA_n(r.H); }
void SLA_L() { r.L = SLA_n(r.L); }
void SLA_aHL() { write(r.HL, SLA_n(read(r.HL))); }

static inline uint8_t SRA_n(register uint8_t n) {
	r.F_C = n & 1;
	n=((char)n)>>1;
	r.F_Z = !n;
	return n;
}

void SRA_A() { r.A = SRA_n(r.A); }
void SRA_B() { r.B = SRA_n(r.B); }
void SRA_C() { r.C = SRA_n(r.C); }
void SRA_D() { r.D = SRA_n(r.D); }
void SRA_E() { r.E = SRA_n(r.E); }
void SRA_H() { r.H = SRA_n(r.H); }
void SRA_L() { r.L = SRA_n(r.L); }
void SRA_aHL() { write(r.HL, SRA_n(read(r.HL))); }

static inline uint8_t SRL_n(register uint8_t n) {
	r.F_C = n & 1;
	n >>= 1;
	r.F_Z = !n;
	return n;
}

void SRL_A() { r.A = SRL_n(r.A); }
void SRL_B() { r.B = SRL_n(r.B); }
void SRL_C() { r.C = SRL_n(r.C); }
void SRL_D() { r.D = SRL_n(r.D); }
void SRL_E() { r.E = SRL_n(r.E); }
void SRL_H() { r.H = SRL_n(r.H); }
void SRL_L() { r.L = SRL_n(r.L); }
void SRL_aHL() { write(r.HL, SRL_n(read(r.HL))); }

static inline void BIT_b_r(register uint8_t b, register uint8_t x) {
	r.F_Z = !(x & (1 << b));
	r.F_N = 0;
	r.F_H = 1;
}

void BIT_b_A(uint8_t b) { BIT_b_r(b, r.A); }
void BIT_b_B(uint8_t b) { BIT_b_r(b, r.B); }
void BIT_b_C(uint8_t b) { BIT_b_r(b, r.C); }
void BIT_b_D(uint8_t b) { BIT_b_r(b, r.D); }
void BIT_b_E(uint8_t b) { BIT_b_r(b, r.E); }
void BIT_b_H(uint8_t b) { BIT_b_r(b, r.H); }
void BIT_b_L(uint8_t b) { BIT_b_r(b, r.L); }
void BIT_b_aHL(uint8_t b) { BIT_b_r(b, read(r.HL)); }

#define RES_b_r(b,x) x&=~(1<<b)
void RES_b_A(uint8_t b) { RES_b_r(b, r.A); }
void RES_b_B(uint8_t b) { RES_b_r(b, r.B); }
void RES_b_C(uint8_t b) { RES_b_r(b, r.C); }
void RES_b_D(uint8_t b) { RES_b_r(b, r.D); }
void RES_b_E(uint8_t b) { RES_b_r(b, r.E); }
void RES_b_H(uint8_t b) { RES_b_r(b, r.H); }
void RES_b_L(uint8_t b) { RES_b_r(b, r.L); }
void RES_b_aHL(uint8_t b) { write(r.HL,read(r.HL)&~(1<<b)); }

#define SET_b_r(b,x) x|=(1<<b)
void SET_b_A(uint8_t b) { SET_b_r(b, r.A); }
void SET_b_B(uint8_t b) { SET_b_r(b, r.B); }
void SET_b_C(uint8_t b) { SET_b_r(b, r.C); }
void SET_b_D(uint8_t b) { SET_b_r(b, r.D); }
void SET_b_E(uint8_t b) { SET_b_r(b, r.E); }
void SET_b_H(uint8_t b) { SET_b_r(b, r.H); }
void SET_b_L(uint8_t b) { SET_b_r(b, r.L); }
void SET_b_aHL(uint8_t b) { write(r.HL,read(r.HL)|(1<<b)); }
