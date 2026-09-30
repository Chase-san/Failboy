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

#include <stdio.h>
#include <stdlib.h>

#include "cpu_instr.h"
#include "cpu_instr_cb.h"
#include "failboy.h"

typedef void (*instruction_f)(void);
typedef void (*instruction_cb_f)(uint8_t);

struct registers r;

uint64_t cycle_counter = 0;

uint8_t ime = 0;      /* interrupt master enable */
uint8_t ei_delay = 0; /* EI takes effect after the following instruction */
uint8_t halted = 0;
uint8_t cpu_locked = 0; /* an illegal opcode hangs the real CPU */

void NOP(void) {}

void XXX(void) {
  uint16_t pc = r.PC - 1;
  fprintf(stderr, "illegal opcode %02X at %04X\n", mem_read(pc), pc);
  cpu_locked = 1;
}

static const instruction_f instr_cb_map[64] = {
    RLC_B,  RLC_C,  RLC_D,  RLC_E,  RLC_H,  RLC_L,  RLC_aHL,  RLC_A,  /* 00-07 */
    RRC_B,  RRC_C,  RRC_D,  RRC_E,  RRC_H,  RRC_L,  RRC_aHL,  RRC_A,  /* 08-0f */
    RL_B,   RL_C,   RL_D,   RL_E,   RL_H,   RL_L,   RL_aHL,   RL_A,   /* 10-17 */
    RR_B,   RR_C,   RR_D,   RR_E,   RR_H,   RR_L,   RR_aHL,   RR_A,   /* 18-1f */
    SLA_B,  SLA_C,  SLA_D,  SLA_E,  SLA_H,  SLA_L,  SLA_aHL,  SLA_A,  /* 20-27 */
    SRA_B,  SRA_C,  SRA_D,  SRA_E,  SRA_H,  SRA_L,  SRA_aHL,  SRA_A,  /* 28-2f */
    SWAP_B, SWAP_C, SWAP_D, SWAP_E, SWAP_H, SWAP_L, SWAP_aHL, SWAP_A, /* 30-37 */
    SRL_B,  SRL_C,  SRL_D,  SRL_E,  SRL_H,  SRL_L,  SRL_aHL,  SRL_A,  /* 38-3f */
};

static const instruction_cb_f instr_cb_bit_map[8] = {
    BIT_b_B, BIT_b_C, BIT_b_D, BIT_b_E, BIT_b_H, BIT_b_L, BIT_b_aHL, BIT_b_A,
};
static const instruction_cb_f instr_cb_res_map[8] = {
    RES_b_B, RES_b_C, RES_b_D, RES_b_E, RES_b_H, RES_b_L, RES_b_aHL, RES_b_A,
};
static const instruction_cb_f instr_cb_set_map[8] = {
    SET_b_B, SET_b_C, SET_b_D, SET_b_E, SET_b_H, SET_b_L, SET_b_aHL, SET_b_A,
};

static const uint8_t instr_cb_timing[256] = {
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCB00 to 0xCB0F
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCB10 to 0xCB1F
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCB20 to 0xCB2F
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCB30 to 0xCB3F
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,  // 0xCB40 to 0xCB4F
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,  // 0xCB50 to 0xCB5F
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,  // 0xCB60 to 0xCB6F
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,  // 0xCB70 to 0xCB7F
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCB80 to 0xCB8F
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCB90 to 0xCB9F
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCBA0 to 0xCBAF
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCBB0 to 0xCBBF
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCBC0 to 0xCBCF
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCBD0 to 0xCBDF
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCBE0 to 0xCBEF
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0xCBF0 to 0xCBFF
};

static inline void step_cb(void) {
  uint8_t op = rpc8_ext();
  if (op < 0x40) {
    instr_cb_map[op]();
    cycle_counter += instr_cb_timing[op] << M_CYCLE_SHL;
    return;
  }
  if (op < 0x80) { /* BIT */
    instr_cb_bit_map[op & 7]((op >> 3) & 7);
    cycle_counter += instr_cb_timing[op] << M_CYCLE_SHL;
    return;
  }
  if (op < 0xC0) { /* RES */
    instr_cb_res_map[op & 7]((op >> 3) & 7);
    cycle_counter += instr_cb_timing[op] << M_CYCLE_SHL;
    return;
  }
  /* SET */
  instr_cb_set_map[op & 7]((op >> 3) & 7);
  cycle_counter += instr_cb_timing[op] << M_CYCLE_SHL;
}

static const instruction_f instr_map[256] = {
    NOP,       LD_BC_nn,  LD_aBC_A,  INC_BC,   INC_B,      DEC_B,    LD_B_n,    RLCA,     /* 00-07 */
    LD_ann_SP, ADD_HL_BC, LD_A_aBC,  DEC_BC,   INC_C,      DEC_C,    LD_C_n,    RRCA,     /* 08-0f */
    STOP,      LD_DE_nn,  LD_aDE_A,  INC_DE,   INC_D,      DEC_D,    LD_D_n,    RLA,      /* 10-17 */
    JR_n,      ADD_HL_DE, LD_A_aDE,  DEC_DE,   INC_E,      DEC_E,    LD_E_n,    RRA,      /* 18-1f */
    JR_NZ_n,   LD_HL_nn,  LDI_aHL_A, INC_HL,   INC_H,      DEC_H,    LD_H_n,    DAA,      /* 20-27 */
    JR_Z_n,    ADD_HL_HL, LDI_A_aHL, DEC_HL,   INC_L,      DEC_L,    LD_L_n,    CPL,      /* 28-2f */
    JR_NC_n,   LD_SP_nn,  LDD_aHL_A, INC_SP,   INC_aHL,    DEC_aHL,  LD_aHL_n,  SCF,      /* 30-37 */
    JR_C_n,    ADD_HL_SP, LDD_A_aHL, DEC_SP,   INC_A,      DEC_A,    LD_A_n,    CCF,      /* 38-3f */
    LD_B_B,    LD_B_C,    LD_B_D,    LD_B_E,   LD_B_H,     LD_B_L,   LD_B_aHL,  LD_B_A,   /* 40-47 */
    LD_C_B,    LD_C_C,    LD_C_D,    LD_C_E,   LD_C_H,     LD_C_L,   LD_C_aHL,  LD_C_A,   /* 48-4f */
    LD_D_B,    LD_D_C,    LD_D_D,    LD_D_E,   LD_D_H,     LD_D_L,   LD_D_aHL,  LD_D_A,   /* 50-57 */
    LD_E_B,    LD_E_C,    LD_E_D,    LD_E_E,   LD_E_H,     LD_E_L,   LD_E_aHL,  LD_E_A,   /* 58-5f */
    LD_H_B,    LD_H_C,    LD_H_D,    LD_H_E,   LD_H_H,     LD_H_L,   LD_H_aHL,  LD_H_A,   /* 60-67 */
    LD_L_B,    LD_L_C,    LD_L_D,    LD_L_E,   LD_L_H,     LD_L_L,   LD_L_aHL,  LD_L_A,   /* 68-6f */
    LD_aHL_B,  LD_aHL_C,  LD_aHL_D,  LD_aHL_E, LD_aHL_H,   LD_aHL_L, HALT,      LD_aHL_A, /* 70-77 */
    LD_A_B,    LD_A_C,    LD_A_D,    LD_A_E,   LD_A_H,     LD_A_L,   LD_A_aHL,  LD_A_A,   /* 78-7f */
    ADD_A_B,   ADD_A_C,   ADD_A_D,   ADD_A_E,  ADD_A_H,    ADD_A_L,  ADD_A_aHL, ADD_A_A,  /* 80-87 */
    ADC_A_B,   ADC_A_C,   ADC_A_D,   ADC_A_E,  ADC_A_H,    ADC_A_L,  ADC_A_aHL, ADC_A_A,  /* 88-8f */
    SUB_B,     SUB_C,     SUB_D,     SUB_E,    SUB_H,      SUB_L,    SUB_aHL,   SUB_A,    /* 90-97 */
    SBC_A_B,   SBC_A_C,   SBC_A_D,   SBC_A_E,  SBC_A_H,    SBC_A_L,  SBC_A_aHL, SBC_A_A,  /* 98-9f */
    AND_B,     AND_C,     AND_D,     AND_E,    AND_H,      AND_L,    AND_aHL,   AND_A,    /* a0-a7 */
    XOR_B,     XOR_C,     XOR_D,     XOR_E,    XOR_H,      XOR_L,    XOR_aHL,   XOR_A,    /* a8-af */
    OR_B,      OR_C,      OR_D,      OR_E,     OR_H,       OR_L,     OR_aHL,    OR_A,     /* b0-b7 */
    CP_B,      CP_C,      CP_D,      CP_E,     CP_H,       CP_L,     CP_aHL,    CP_A,     /* b8-bf */
    RET_NZ,    POP_BC,    JP_NZ,     JP,       CALL_NZ_nn, PUSH_BC,  ADD_A_n,   RST00,    /* c0-c7 */
    RET_Z,     RET,       JP_Z,      step_cb,  CALL_Z_nn,  CALL_nn,  ADC_A_n,   RST08,    /* c8-cf */
    RET_NC,    POP_DE,    JP_NC,     XXX,      CALL_NC_nn, PUSH_DE,  SUB_n,     RST10,    /* d0-d7 */
    RET_C,     RETI,      JP_C,      XXX,      CALL_C_nn,  XXX,      SBC_A_n,   RST18,    /* d8-df */
    LDH_an_A,  POP_HL,    LD_aC_A,   XXX,      XXX,        PUSH_HL,  AND_n,     RST20,    /* e0-e7 */
    ADD_SP_n,  JP_HL,     LD_ann_A,  XXX,      XXX,        XXX,      XOR_n,     RST28,    /* e8-ef */
    LDH_A_an,  POP_AF,    LD_A_aC,   DI,       XXX,        PUSH_AF,  OR_n,      RST30,    /* f0-f7 */
    LDHL_SP_n, LD_SP_HL,  LD_A_ann,  EI,       XXX,        XXX,      CP_n,      RST38,    /* f8-ff */
};

/* M-cycles. Conditional branches list the not-taken cost; see cpu_instr.c. */
static const uint8_t instr_timing[256] = {
    1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1,  // 0x00 to 0x0F
    1, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1,  // 0x10 to 0x1F
    2, 3, 2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1,  // 0x20 to 0x2F
    2, 3, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 1, 1, 2, 1,  // 0x30 to 0x3F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 0x40 to 0x4F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 0x50 to 0x5F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 0x60 to 0x6F
    2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1,  // 0x70 to 0x7F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 0x80 to 0x8F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 0x90 to 0x9F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 0xA0 to 0xAF
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 0xB0 to 0xBF
    2, 3, 3, 4, 3, 4, 2, 4, 2, 4, 3, 0, 3, 6, 2, 4,  // 0xC0 to 0xCF
    2, 3, 3, 0, 3, 4, 2, 4, 2, 4, 3, 0, 3, 0, 2, 4,  // 0xD0 to 0xDF
    3, 3, 2, 0, 0, 4, 2, 4, 4, 1, 4, 0, 0, 0, 2, 4,  // 0xE0 to 0xEF
    3, 3, 2, 1, 0, 4, 2, 4, 3, 2, 4, 1, 0, 0, 2, 4,  // 0xF0 to 0xFF
};

/* in IF/IE bit order: VBlank, STAT, timer, serial, joypad */
static const uint16_t interrupt_vector[5] = {
    0x40, 0x48, 0x50, 0x58, 0x60,
};

/* Wake from HALT on any pending interrupt; with IME set, jump to the highest-priority one. */
static void cpu_interrupt(void) {
  uint8_t pending = IO_REG(IO_IF) & io_ie & INT_ALL;
  if (!pending) {
    return;
  }
  halted = 0;
  if (!ime) {
    return;
  }
  /* the lowest set bit wins */
  uint8_t n = 0;
  while (!(pending & (1 << n))) {
    ++n;
  }
  ime = 0;
  IO_REG(IO_IF) &= ~(1 << n);
  push16_ext(r.PC);
  r.PC = interrupt_vector[n];
  cycle_counter += 5 << M_CYCLE_SHL;
}

uint32_t step(void) {
  uint64_t start = cycle_counter;
  cpu_interrupt();
  if (halted) {
    cycle_counter += M_CYCLE;
  } else {
    uint8_t op = rpc8_ext();
    instr_map[op]();
    cycle_counter += instr_timing[op] << M_CYCLE_SHL;
    /* EI enables interrupts only after the instruction that follows it */
    if (ei_delay && --ei_delay == 0) {
      ime = 1;
    }
    if (doctor) {
      cpu_trace();
    }
  }

  uint32_t cycles = (uint32_t)(cycle_counter - start);
  io_tick(cycles);
  if (!doctor) {
    video_tick(cycles);
  }
  return cycles;
}

/* One line per instruction, in Gameboy Doctor's log format. */
void cpu_trace(void) {
  printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n", r.A,
         r.F, r.B, r.C, r.D, r.E, r.H, r.L, r.SP, r.PC, mem_read(r.PC), mem_read(r.PC + 1), mem_read(r.PC + 2),
         mem_read(r.PC + 3));
}

void cpu_bios_init(void) {
  /* Bit-field layout is up to the compiler; make sure F_Z..F_C really are bits 7..4. */
  r.F = 0x90;
  if (!r.F_Z || r.F_N || r.F_H || !r.F_C) {
    fprintf(stderr, "flag bit-fields don't match the F register layout\n");
    abort();
  }

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
  ime = ei_delay = halted = cpu_locked = 0;

  mem_write(0xFF05, 0x00);  // TIMA
  mem_write(0xFF06, 0x00);  // TMA
  mem_write(0xFF07, 0x00);  // TAC
  mem_write(0xFF0F, 0xE1);  // IF
  mem_write(0xFF10, 0x80);  // NR10
  mem_write(0xFF11, 0xBF);  // NR11
  mem_write(0xFF12, 0xF3);  // NR12
  mem_write(0xFF14, 0xBF);  // NR14
  mem_write(0xFF16, 0x3F);  // NR21
  mem_write(0xFF17, 0x00);  // NR22
  mem_write(0xFF19, 0xBF);  // NR24
  mem_write(0xFF1A, 0x7F);  // NR30
  mem_write(0xFF1B, 0xFF);  // NR31
  mem_write(0xFF1C, 0x9F);  // NR32
  mem_write(0xFF1E, 0xBF);  // NR33
  mem_write(0xFF20, 0xFF);  // NR41
  mem_write(0xFF21, 0x00);  // NR42
  mem_write(0xFF22, 0x00);  // NR43
  mem_write(0xFF23, 0xBF);  // NR44
  mem_write(0xFF24, 0x77);  // NR50
  mem_write(0xFF25, 0xF3);  // NR51
  mem_write(0xFF26, 0xF1);  // NR52 // 0xF1 GB, 0xF0 SGB
  mem_write(0xFF40, 0x91);  // LCDC
  mem_write(0xFF42, 0x00);  // SCY
  mem_write(0xFF43, 0x00);  // SCX
  mem_write(0xFF45, 0x00);  // LYC
  mem_write(0xFF47, 0xFC);  // BGP
  mem_write(0xFF48, 0xFF);  // OBP0
  mem_write(0xFF49, 0xFF);  // OBP1
  mem_write(0xFF4A, 0x00);  // WY
  mem_write(0xFF4B, 0x00);  // WX
  mem_write(0xFFFF, 0x00);  // IE
}
