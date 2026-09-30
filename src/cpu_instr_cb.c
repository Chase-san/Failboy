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

#include "cpu_instr_cb.h"

#include "failboy.h"

/* CB Instructions */
static force_inline uint8_t SWAP(uint8_t n) {
  r.F = 0;
  r.F_Z = n == 0;
  return (n >> 4) | (n << 4);
}

void SWAP_A(void) { r.A = SWAP(r.A); }

void SWAP_B(void) { r.B = SWAP(r.B); }

void SWAP_C(void) { r.C = SWAP(r.C); }

void SWAP_D(void) { r.D = SWAP(r.D); }

void SWAP_E(void) { r.E = SWAP(r.E); }

void SWAP_H(void) { r.H = SWAP(r.H); }

void SWAP_L(void) { r.L = SWAP(r.L); }

void SWAP_aHL(void) { mem_write(r.HL, SWAP(mem_read(r.HL))); }

static force_inline uint8_t RLC_n(register uint8_t n) {
  register uint8_t bit = (n >> 7) & 1;
  n = (n << 1) | bit;
  r.F = 0;
  r.F_C = bit;
  r.F_Z = !n;
  return n;
}

void RLC_A(void) { r.A = RLC_n(r.A); }

void RLC_B(void) { r.B = RLC_n(r.B); }

void RLC_C(void) { r.C = RLC_n(r.C); }

void RLC_D(void) { r.D = RLC_n(r.D); }

void RLC_E(void) { r.E = RLC_n(r.E); }

void RLC_H(void) { r.H = RLC_n(r.H); }

void RLC_L(void) { r.L = RLC_n(r.L); }

void RLC_aHL(void) { mem_write(r.HL, RLC_n(mem_read(r.HL))); }

static force_inline uint8_t RL_n(register uint8_t n) {
  register uint8_t bit = (n >> 7) & 1;
  n = (n << 1) | r.F_C;
  r.F = 0;
  r.F_C = bit;
  r.F_Z = !n;
  return n;
}

void RL_A(void) { r.A = RL_n(r.A); }

void RL_B(void) { r.B = RL_n(r.B); }

void RL_C(void) { r.C = RL_n(r.C); }

void RL_D(void) { r.D = RL_n(r.D); }

void RL_E(void) { r.E = RL_n(r.E); }

void RL_H(void) { r.H = RL_n(r.H); }

void RL_L(void) { r.L = RL_n(r.L); }

void RL_aHL(void) { mem_write(r.HL, RL_n(mem_read(r.HL))); }

static force_inline uint8_t RRC_n(register uint8_t n) {
  register uint8_t bit = n & 1;
  n = (n >> 1) | (bit << 7);
  r.F = 0;
  r.F_C = bit;
  r.F_Z = !n;
  return n;
}

void RRC_A(void) { r.A = RRC_n(r.A); }

void RRC_B(void) { r.B = RRC_n(r.B); }

void RRC_C(void) { r.C = RRC_n(r.C); }

void RRC_D(void) { r.D = RRC_n(r.D); }

void RRC_E(void) { r.E = RRC_n(r.E); }

void RRC_H(void) { r.H = RRC_n(r.H); }

void RRC_L(void) { r.L = RRC_n(r.L); }

void RRC_aHL(void) { mem_write(r.HL, RRC_n(mem_read(r.HL))); }

static force_inline uint8_t RR_n(register uint8_t n) {
  register uint8_t bit = n & 1;
  n = (n >> 1) | (r.F_C << 7);
  r.F = 0;
  r.F_C = bit;
  r.F_Z = !n;
  return n;
}

void RR_A(void) { r.A = RR_n(r.A); }

void RR_B(void) { r.B = RR_n(r.B); }

void RR_C(void) { r.C = RR_n(r.C); }

void RR_D(void) { r.D = RR_n(r.D); }

void RR_E(void) { r.E = RR_n(r.E); }

void RR_H(void) { r.H = RR_n(r.H); }

void RR_L(void) { r.L = RR_n(r.L); }

void RR_aHL(void) { mem_write(r.HL, RR_n(mem_read(r.HL))); }

static force_inline uint8_t SLA_n(register uint8_t n) {
  register uint8_t bit = (n >> 7) & 1;
  n <<= 1;
  r.F = 0;
  r.F_C = bit;
  r.F_Z = !n;
  return n;
}

void SLA_A(void) { r.A = SLA_n(r.A); }

void SLA_B(void) { r.B = SLA_n(r.B); }

void SLA_C(void) { r.C = SLA_n(r.C); }

void SLA_D(void) { r.D = SLA_n(r.D); }

void SLA_E(void) { r.E = SLA_n(r.E); }

void SLA_H(void) { r.H = SLA_n(r.H); }

void SLA_L(void) { r.L = SLA_n(r.L); }

void SLA_aHL(void) { mem_write(r.HL, SLA_n(mem_read(r.HL))); }

static force_inline uint8_t SRA_n(register uint8_t n) {
  register uint8_t bit = n & 1;
  n = (n >> 1) | (n & 0x80); /* bit 7 stays put */
  r.F = 0;
  r.F_C = bit;
  r.F_Z = !n;
  return n;
}

void SRA_A(void) { r.A = SRA_n(r.A); }

void SRA_B(void) { r.B = SRA_n(r.B); }

void SRA_C(void) { r.C = SRA_n(r.C); }

void SRA_D(void) { r.D = SRA_n(r.D); }

void SRA_E(void) { r.E = SRA_n(r.E); }

void SRA_H(void) { r.H = SRA_n(r.H); }

void SRA_L(void) { r.L = SRA_n(r.L); }

void SRA_aHL(void) { mem_write(r.HL, SRA_n(mem_read(r.HL))); }

static force_inline uint8_t SRL_n(register uint8_t n) {
  register uint8_t bit = n & 1;
  n >>= 1;
  r.F = 0;
  r.F_C = bit;
  r.F_Z = !n;
  return n;
}

void SRL_A(void) { r.A = SRL_n(r.A); }

void SRL_B(void) { r.B = SRL_n(r.B); }

void SRL_C(void) { r.C = SRL_n(r.C); }

void SRL_D(void) { r.D = SRL_n(r.D); }

void SRL_E(void) { r.E = SRL_n(r.E); }

void SRL_H(void) { r.H = SRL_n(r.H); }

void SRL_L(void) { r.L = SRL_n(r.L); }

void SRL_aHL(void) { mem_write(r.HL, SRL_n(mem_read(r.HL))); }

static force_inline void BIT_b_r(register uint8_t b, register uint8_t x) {
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

void BIT_b_aHL(uint8_t b) { BIT_b_r(b, mem_read(r.HL)); }

#define RES_b_r(b, x) x &= ~(1 << b)

void RES_b_A(uint8_t b) { RES_b_r(b, r.A); }

void RES_b_B(uint8_t b) { RES_b_r(b, r.B); }

void RES_b_C(uint8_t b) { RES_b_r(b, r.C); }

void RES_b_D(uint8_t b) { RES_b_r(b, r.D); }

void RES_b_E(uint8_t b) { RES_b_r(b, r.E); }

void RES_b_H(uint8_t b) { RES_b_r(b, r.H); }

void RES_b_L(uint8_t b) { RES_b_r(b, r.L); }

void RES_b_aHL(uint8_t b) { mem_write(r.HL, mem_read(r.HL) & ~(1 << b)); }

#define SET_b_r(b, x) x |= (1 << b)

void SET_b_A(uint8_t b) { SET_b_r(b, r.A); }

void SET_b_B(uint8_t b) { SET_b_r(b, r.B); }

void SET_b_C(uint8_t b) { SET_b_r(b, r.C); }

void SET_b_D(uint8_t b) { SET_b_r(b, r.D); }

void SET_b_E(uint8_t b) { SET_b_r(b, r.E); }

void SET_b_H(uint8_t b) { SET_b_r(b, r.H); }

void SET_b_L(uint8_t b) { SET_b_r(b, r.L); }

void SET_b_aHL(uint8_t b) { mem_write(r.HL, mem_read(r.HL) | (1 << b)); }
