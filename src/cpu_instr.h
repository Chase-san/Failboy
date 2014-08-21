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

/* Defines the general CPU instructions. */

/* 8-bit Loads */
void LD_A_n();
void LD_B_n();
void LD_C_n();
void LD_D_n();
void LD_E_n();
void LD_H_n();
void LD_L_n();
void LD_aHL_n();
void LD_A_A();
void LD_A_B();
void LD_A_C();
void LD_A_D();
void LD_A_E();
void LD_A_H();
void LD_A_L();
void LD_A_aHL();
void LD_A_aC();
void LD_aC_A();
void LD_A_aBC();
void LD_A_aDE();
void LD_A_ann();
void LD_B_A();
void LD_B_B();
void LD_B_C();
void LD_B_D();
void LD_B_E();
void LD_B_H();
void LD_B_L();
void LD_B_aHL();
void LD_C_A();
void LD_C_B();
void LD_C_C();
void LD_C_D();
void LD_C_E();
void LD_C_H();
void LD_C_L();
void LD_C_aHL();
void LD_D_A();
void LD_D_B();
void LD_D_C();
void LD_D_D();
void LD_D_E();
void LD_D_H();
void LD_D_L();
void LD_D_aHL();
void LD_E_A();
void LD_E_B();
void LD_E_C();
void LD_E_D();
void LD_E_E();
void LD_E_H();
void LD_E_L();
void LD_E_aHL();
void LD_H_A();
void LD_H_B();
void LD_H_C();
void LD_H_D();
void LD_H_E();
void LD_H_H();
void LD_H_L();
void LD_H_aHL();
void LD_L_A();
void LD_L_B();
void LD_L_C();
void LD_L_D();
void LD_L_E();
void LD_L_H();
void LD_L_L();
void LD_L_aHL();
void LD_aHL_A();
void LD_aHL_B();
void LD_aHL_C();
void LD_aHL_D();
void LD_aHL_E();
void LD_aHL_H();
void LD_aHL_L();
void LD_aBC_A();
void LD_aDE_A();
void LD_ann_A();
void LDD_A_aHL();
void LDD_aHL_A();
void LDI_A_aHL();
void LDI_aHL_A();
void LDH_A_an();
void LDH_an_A();

/* 16-bit loads */
void LD_BC_nn();
void LD_DE_nn();
void LD_HL_nn();
void LD_SP_nn();
void LD_SP_HL();
void LDHL_SP_n();
void LD_ann_SP();
void PUSH_AF();
void PUSH_BC();
void PUSH_DE();
void PUSH_HL();
void POP_AF();
void POP_BC();
void POP_DE();
void POP_HL();

/* 8-bit Arithmetic (ALU8) */
void ADD_A_A();
void ADD_A_B();
void ADD_A_C();
void ADD_A_D();
void ADD_A_E();
void ADD_A_H();
void ADD_A_L();
void ADD_A_aHL();
void ADD_A_n();
void ADC_A_A();
void ADC_A_B();
void ADC_A_C();
void ADC_A_D();
void ADC_A_E();
void ADC_A_H();
void ADC_A_L();
void ADC_A_aHL();
void ADC_A_n();
void SUB_A();
void SUB_B();
void SUB_C();
void SUB_D();
void SUB_E();
void SUB_H();
void SUB_L();
void SUB_aHL();
void SUB_n();
void SBC_A_A();
void SBC_A_B();
void SBC_A_C();
void SBC_A_D();
void SBC_A_E();
void SBC_A_H();
void SBC_A_L();
void SBC_A_aHL();
void SBC_A_n();
void AND_A();
void AND_B();
void AND_C();
void AND_D();
void AND_E();
void AND_H();
void AND_L();
void AND_aHL();
void AND_n();
void OR_A();
void OR_B();
void OR_C();
void OR_D();
void OR_E();
void OR_H();
void OR_L();
void OR_aHL();
void OR_n();
void XOR_A();
void XOR_B();
void XOR_C();
void XOR_D();
void XOR_E();
void XOR_H();
void XOR_L();
void XOR_aHL();
void XOR_n();
void CP_A();
void CP_B();
void CP_C();
void CP_D();
void CP_E();
void CP_H();
void CP_L();
void CP_aHL();
void CP_n();
void INC_A();
void INC_B();
void INC_C();
void INC_D();
void INC_E();
void INC_H();
void INC_L();
void INC_aHL();
void DEC_A();
void DEC_B();
void DEC_C();
void DEC_D();
void DEC_E();
void DEC_H();
void DEC_L();
void DEC_aHL();

/* 16-bit Arithmetic (ALU16) */
void ADD_HL_BC();
void ADD_HL_DE();
void ADD_HL_HL();
void ADD_HL_SP();
void ADD_SP_n();
void INC_BC();
void INC_DE();
void INC_HL();
void INC_SP();
void DEC_BC();
void DEC_DE();
void DEC_HL();
void DEC_SP();

/* Rotates & Shifts */
void RLCA();
void RLA();
void RRCA();
void RRA();

/* Jumps */
void JP();
void JP_NZ();
void JP_Z();
void JP_NC();
void JP_C();
void JP_HL();
void JR_n();
void JR_NZ_n();
void JR_Z_n();
void JR_NC_n();
void JR_C_n();

/* Calls */
void CALL_nn();
void CALL_NZ_nn();
void CALL_Z_nn();
void CALL_NC_nn();
void CALL_C_nn();

/* Restarts */
void RST00();
void RST08();
void RST10();
void RST18();
void RST20();
void RST28();
void RST30();
void RST38();

/* Misc */
void CPL();
void CCF();
void SCF();
void DAA();
void HALT();
void STOP();
void DI();
void EI();

/* Return */
void RET();
void RET_NZ();
void RET_Z();
void RET_NC();
void RET_C();
void RETI();

#endif /* _CPU_INSTR_H_ */
