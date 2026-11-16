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

#include <stdint.h>

/* This file defines the DMG two byte 0xCB instructions. */
void SWAP_A(void);
void SWAP_B(void);
void SWAP_C(void);
void SWAP_D(void);
void SWAP_E(void);
void SWAP_H(void);
void SWAP_L(void);
void SWAP_aHL(void);

void RLC_A(void);
void RLC_B(void);
void RLC_C(void);
void RLC_D(void);
void RLC_E(void);
void RLC_H(void);
void RLC_L(void);
void RLC_aHL(void);

void RL_A(void);
void RL_B(void);
void RL_C(void);
void RL_D(void);
void RL_E(void);
void RL_H(void);
void RL_L(void);
void RL_aHL(void);

void RRC_A(void);
void RRC_B(void);
void RRC_C(void);
void RRC_D(void);
void RRC_E(void);
void RRC_H(void);
void RRC_L(void);
void RRC_aHL(void);

void RR_A(void);
void RR_B(void);
void RR_C(void);
void RR_D(void);
void RR_E(void);
void RR_H(void);
void RR_L(void);
void RR_aHL(void);

void SLA_A(void);
void SLA_B(void);
void SLA_C(void);
void SLA_D(void);
void SLA_E(void);
void SLA_H(void);
void SLA_L(void);
void SLA_aHL(void);

void SRA_A(void);
void SRA_B(void);
void SRA_C(void);
void SRA_D(void);
void SRA_E(void);
void SRA_H(void);
void SRA_L(void);
void SRA_aHL(void);

void SRL_A(void);
void SRL_B(void);
void SRL_C(void);
void SRL_D(void);
void SRL_E(void);
void SRL_H(void);
void SRL_L(void);
void SRL_aHL(void);

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
