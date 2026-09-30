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

#ifndef _CPU_INSTR_H_
#define _CPU_INSTR_H_

#include <stdint.h>

/* Defines the general CPU instructions. */

/* Operand fetch and stack helpers */
uint8_t rpc8_ext(void);
uint16_t rpc16_ext(void);

/* 8-bit Loads */
void LD_A_n(void);
void LD_B_n(void);
void LD_C_n(void);
void LD_D_n(void);
void LD_E_n(void);
void LD_H_n(void);
void LD_L_n(void);
void LD_aHL_n(void);
void LD_A_A(void);
void LD_A_B(void);
void LD_A_C(void);
void LD_A_D(void);
void LD_A_E(void);
void LD_A_H(void);
void LD_A_L(void);
void LD_A_aHL(void);
void LD_A_aC(void);
void LD_aC_A(void);
void LD_A_aBC(void);
void LD_A_aDE(void);
void LD_A_ann(void);
void LD_B_A(void);
void LD_B_B(void);
void LD_B_C(void);
void LD_B_D(void);
void LD_B_E(void);
void LD_B_H(void);
void LD_B_L(void);
void LD_B_aHL(void);
void LD_C_A(void);
void LD_C_B(void);
void LD_C_C(void);
void LD_C_D(void);
void LD_C_E(void);
void LD_C_H(void);
void LD_C_L(void);
void LD_C_aHL(void);
void LD_D_A(void);
void LD_D_B(void);
void LD_D_C(void);
void LD_D_D(void);
void LD_D_E(void);
void LD_D_H(void);
void LD_D_L(void);
void LD_D_aHL(void);
void LD_E_A(void);
void LD_E_B(void);
void LD_E_C(void);
void LD_E_D(void);
void LD_E_E(void);
void LD_E_H(void);
void LD_E_L(void);
void LD_E_aHL(void);
void LD_H_A(void);
void LD_H_B(void);
void LD_H_C(void);
void LD_H_D(void);
void LD_H_E(void);
void LD_H_H(void);
void LD_H_L(void);
void LD_H_aHL(void);
void LD_L_A(void);
void LD_L_B(void);
void LD_L_C(void);
void LD_L_D(void);
void LD_L_E(void);
void LD_L_H(void);
void LD_L_L(void);
void LD_L_aHL(void);
void LD_aHL_A(void);
void LD_aHL_B(void);
void LD_aHL_C(void);
void LD_aHL_D(void);
void LD_aHL_E(void);
void LD_aHL_H(void);
void LD_aHL_L(void);
void LD_aBC_A(void);
void LD_aDE_A(void);
void LD_ann_A(void);
void LDD_A_aHL(void);
void LDD_aHL_A(void);
void LDI_A_aHL(void);
void LDI_aHL_A(void);
void LDH_A_an(void);
void LDH_an_A(void);

/* 16-bit loads */
void LD_BC_nn(void);
void LD_DE_nn(void);
void LD_HL_nn(void);
void LD_SP_nn(void);
void LD_SP_HL(void);
void LDHL_SP_n(void);
void LD_ann_SP(void);
void PUSH_AF(void);
void PUSH_BC(void);
void PUSH_DE(void);
void PUSH_HL(void);
void POP_AF(void);
void POP_BC(void);
void POP_DE(void);
void POP_HL(void);

/* 8-bit Arithmetic (ALU8) */
void ADD_A_A(void);
void ADD_A_B(void);
void ADD_A_C(void);
void ADD_A_D(void);
void ADD_A_E(void);
void ADD_A_H(void);
void ADD_A_L(void);
void ADD_A_aHL(void);
void ADD_A_n(void);
void ADC_A_A(void);
void ADC_A_B(void);
void ADC_A_C(void);
void ADC_A_D(void);
void ADC_A_E(void);
void ADC_A_H(void);
void ADC_A_L(void);
void ADC_A_aHL(void);
void ADC_A_n(void);
void SUB_A(void);
void SUB_B(void);
void SUB_C(void);
void SUB_D(void);
void SUB_E(void);
void SUB_H(void);
void SUB_L(void);
void SUB_aHL(void);
void SUB_n(void);
void SBC_A_A(void);
void SBC_A_B(void);
void SBC_A_C(void);
void SBC_A_D(void);
void SBC_A_E(void);
void SBC_A_H(void);
void SBC_A_L(void);
void SBC_A_aHL(void);
void SBC_A_n(void);
void AND_A(void);
void AND_B(void);
void AND_C(void);
void AND_D(void);
void AND_E(void);
void AND_H(void);
void AND_L(void);
void AND_aHL(void);
void AND_n(void);
void OR_A(void);
void OR_B(void);
void OR_C(void);
void OR_D(void);
void OR_E(void);
void OR_H(void);
void OR_L(void);
void OR_aHL(void);
void OR_n(void);
void XOR_A(void);
void XOR_B(void);
void XOR_C(void);
void XOR_D(void);
void XOR_E(void);
void XOR_H(void);
void XOR_L(void);
void XOR_aHL(void);
void XOR_n(void);
void CP_A(void);
void CP_B(void);
void CP_C(void);
void CP_D(void);
void CP_E(void);
void CP_H(void);
void CP_L(void);
void CP_aHL(void);
void CP_n(void);
void INC_A(void);
void INC_B(void);
void INC_C(void);
void INC_D(void);
void INC_E(void);
void INC_H(void);
void INC_L(void);
void INC_aHL(void);
void DEC_A(void);
void DEC_B(void);
void DEC_C(void);
void DEC_D(void);
void DEC_E(void);
void DEC_H(void);
void DEC_L(void);
void DEC_aHL(void);

/* 16-bit Arithmetic (ALU16) */
void ADD_HL_BC(void);
void ADD_HL_DE(void);
void ADD_HL_HL(void);
void ADD_HL_SP(void);
void ADD_SP_n(void);
void INC_BC(void);
void INC_DE(void);
void INC_HL(void);
void INC_SP(void);
void DEC_BC(void);
void DEC_DE(void);
void DEC_HL(void);
void DEC_SP(void);

/* Rotates & Shifts */
void RLCA(void);
void RLA(void);
void RRCA(void);
void RRA(void);

/* Jumps */
void JP(void);
void JP_NZ(void);
void JP_Z(void);
void JP_NC(void);
void JP_C(void);
void JP_HL(void);
void JR_n(void);
void JR_NZ_n(void);
void JR_Z_n(void);
void JR_NC_n(void);
void JR_C_n(void);

/* Calls */
void CALL_nn(void);
void CALL_NZ_nn(void);
void CALL_Z_nn(void);
void CALL_NC_nn(void);
void CALL_C_nn(void);

/* Restarts */
void RST00(void);
void RST08(void);
void RST10(void);
void RST18(void);
void RST20(void);
void RST28(void);
void RST30(void);
void RST38(void);

/* Misc */
void CPL(void);
void CCF(void);
void SCF(void);
void DAA(void);
void HALT(void);
void STOP(void);
void DI(void);
void EI(void);

/* Return */
void RET(void);
void RET_NZ(void);
void RET_Z(void);
void RET_NC(void);
void RET_C(void);
void RETI(void);

#endif /* _CPU_INSTR_H_ */
