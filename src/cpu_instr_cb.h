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

#ifndef _CPU_INSTR_CB_H_
#define _CPU_INSTR_CB_H_

/* This file defines the DMG two byte 0xCB instructions. */
void SWAP_A();
void SWAP_B();
void SWAP_C();
void SWAP_D();
void SWAP_E();
void SWAP_H();
void SWAP_L();
void SWAP_aHL();

void RLC_B();
void RLC_C();
void RLC_D();
void RLC_E();
void RLC_H();
void RLC_L();
void RLC_aHL();

void RL_B();
void RL_C();
void RL_D();
void RL_E();
void RL_H();
void RL_L();
void RL_aHL();

void RRC_B();
void RRC_C();
void RRC_D();
void RRC_E();
void RRC_H();
void RRC_L();
void RRC_aHL();

void RR_B();
void RR_C();
void RR_D();
void RR_E();
void RR_H();
void RR_L();
void RR_aHL();

void SLA_A();
void SLA_B();
void SLA_C();
void SLA_D();
void SLA_E();
void SLA_H();
void SLA_L();
void SLA_aHL();

void SRA_A();
void SRA_B();
void SRA_C();
void SRA_D();
void SRA_E();
void SRA_H();
void SRA_L();
void SRA_aHL();

void SRL_A();
void SRL_B();
void SRL_C();
void SRL_D();
void SRL_E();
void SRL_H();
void SRL_L();
void SRL_aHL();

void BIT_b_A(uint8_t);
void BIT_b_B(uint8_t);
void BIT_b_C(uint8_t);
void BIT_b_D(uint8_t);
void BIT_b_E(uint8_t);
void BIT_b_H(uint8_t);
void BIT_b_L(uint8_t);
void BIT_b_aHL(uint8_t);

void RES_b_A(uint8_t b);
void RES_b_B(uint8_t b);
void RES_b_C(uint8_t b);
void RES_b_D(uint8_t b);
void RES_b_E(uint8_t b);
void RES_b_H(uint8_t b);
void RES_b_L(uint8_t b);
void RES_b_aHL(uint8_t b);

void SET_b_A(uint8_t b);
void SET_b_B(uint8_t b);
void SET_b_C(uint8_t b);
void SET_b_D(uint8_t b);
void SET_b_E(uint8_t b);
void SET_b_H(uint8_t b);
void SET_b_L(uint8_t b);
void SET_b_aHL(uint8_t b);

#endif
