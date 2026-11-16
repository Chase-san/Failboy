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

/* A Game Boy Color's speed switch: STOP with KEY1 armed stops the CPU for a while that isn't a whole number of its */
/* M-cycles, restarts the divider, and can be cut short by an interrupt. The figures are the core's (cpu_instr.c), */
/* which has them from other people's tests of the hardware; this is to notice them moving. And what else a STOP */
/* is, by whether a button is down and an interrupt waiting. */

#include <stdlib.h>

#include "tests/test.h"

enum {
  DIV = 0xFF04,
  TIMA = 0xFF05,
  TAC = 0xFF07,
  IF = 0xFF0F,
  KEY1 = 0xFF4D,
  IE = 0xFFFF,
  IF_TIMER = 0x04,
  TAC_FASTEST = 0x05,
  KEY1_ARMED = 0x01,
  KEY1_NORMAL = 0x7E, /* as read: the unused bits set */
  KEY1_DOUBLE = 0xFE,
  M_CYCLE = 4,              /* ticks, at the normal speed */
  STOP_M_CYCLES = 2,        /* the opcode and the byte after it */
  TO_DOUBLE = 0x10002,      /* ticks the CPU waits going to double speed, */
  TO_NORMAL = 0x20003,      /* and coming back */
  DIV_PERIOD_M_CYCLES = 64, /* M-cycles of the CPU to a count of DIV, at either speed */
  SECOND_STOP = 0x0200,     /* where the program's second STOP is */
  THIRD_STOP = 0x0300,
  FOURTH_STOP = 0x0400, /* these three have an INC A after them */
  FIFTH_STOP = 0x0500,
  SIXTH_STOP = 0x0600,
  SEVENTH_STOP = 0x0700, /* and this one an LD A, 0 */
  STOP_GAP = 0x0100,
  P1 = 0xFF00,
  P1_BUTTONS = 0x10, /* written to P1: the buttons' line selected, the directions' not */
  STOP_SIZE = 2,
};

static const uint8_t first_switch[] = {
    0x3E, KEY1_ARMED, /* LD A, 1 */
    0xE0, 0x4D,       /* LDH (KEY1), A */
    0x10, 0x00,       /* STOP */
};

static uint16_t pc(struct gb *gb) {
  struct gb_cpu cpu;
  gb_cpu(gb, &cpu);
  return cpu.pc;
}

static uint8_t accumulator(struct gb *gb) {
  struct gb_cpu cpu;
  gb_cpu(gb, &cpu);
  return (uint8_t)(cpu.af >> 8);
}

static int halted(struct gb *gb) {
  struct gb_cpu cpu;
  gb_cpu(gb, &cpu);
  return cpu.halted;
}

/* runs to where the CPU is about to fetch from an address, and says how many ticks that took */
static uint64_t run_to(struct gb *gb, uint16_t address) {
  uint64_t start = gb_ticks(gb);
  while (pc(gb) != address) {
    gb_step(gb);
  }
  return gb_ticks(gb) - start;
}

static void nops(struct gb *gb, int count) {
  for (int i = 0; i < count; ++i) {
    gb_step(gb);
  }
}

int main(void) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, 0, 0);
  rom[TEST_HEADER_CGB] = TEST_HEADER_CGB_GAME;
  test_program(rom, first_switch, sizeof(first_switch));
  rom[SECOND_STOP] = 0x10;
  rom[THIRD_STOP] = 0x10;
  for (uint16_t stop = FOURTH_STOP; stop <= SIXTH_STOP; stop += STOP_GAP) {
    rom[stop] = 0x10;
    rom[stop + 1] = 0x3C; /* INC A */
  }
  rom[SEVENTH_STOP] = 0x10;
  rom[SEVENTH_STOP + 1] = 0x3E; /* LD A, 0 */
  struct gb *gb = test_boot(GB_CGB_AUTO, rom, TEST_ROM_SIZE, NULL);
  CHECK(gb_read(gb, KEY1) == KEY1_NORMAL);

  /* to double speed: the STOP at the normal speed, then the wait */
  uint16_t stop = TEST_PROGRAM + sizeof(first_switch) - STOP_SIZE;
  run_to(gb, stop);
  CHECK(run_to(gb, stop + STOP_SIZE) == STOP_M_CYCLES * M_CYCLE);
  CHECK(gb_read(gb, KEY1) == KEY1_DOUBLE);
  uint64_t before = gb_ticks(gb);
  gb_step(gb); /* nothing moves the PC until the wait is over, so step it out */
  while (pc(gb) == stop + STOP_SIZE) {
    gb_step(gb);
  }
  /* the wait, and the NOP after it, which takes half the ticks now */
  CHECK(gb_ticks(gb) - before == TO_DOUBLE + M_CYCLE / 2);
  CHECK(gb_read(gb, DIV) == 0);
  nops(gb, DIV_PERIOD_M_CYCLES - 3);
  CHECK(gb_read(gb, DIV) == 0);
  nops(gb, 1);
  CHECK(gb_read(gb, DIV) == 1);

  /* and back: the STOP at double speed, then a wait twice as long */
  run_to(gb, SECOND_STOP - 1);
  gb_write(gb, KEY1, KEY1_ARMED);
  nops(gb, 1);
  CHECK(run_to(gb, SECOND_STOP + STOP_SIZE) == STOP_M_CYCLES * M_CYCLE / 2);
  CHECK(gb_read(gb, KEY1) == KEY1_NORMAL);
  before = gb_ticks(gb);
  gb_step(gb);
  while (pc(gb) == SECOND_STOP + STOP_SIZE) {
    gb_step(gb);
  }
  CHECK(gb_ticks(gb) - before == TO_NORMAL + M_CYCLE);
  CHECK(gb_read(gb, DIV) == 0);
  nops(gb, DIV_PERIOD_M_CYCLES - 3);
  CHECK(gb_read(gb, DIV) == 0);
  nops(gb, 1);
  CHECK(gb_read(gb, DIV) == 1);

  /* an interrupt that comes up during the wait ends it, with the speed switched all the same */
  run_to(gb, THIRD_STOP - 1);
  gb_write(gb, KEY1, KEY1_ARMED);
  gb_write(gb, IF, 0);
  gb_write(gb, IE, IF_TIMER);
  gb_write(gb, TIMA, 0);
  gb_write(gb, TAC, TAC_FASTEST);
  nops(gb, 1);
  run_to(gb, THIRD_STOP + STOP_SIZE);
  before = gb_ticks(gb);
  gb_step(gb);
  while (pc(gb) == THIRD_STOP + STOP_SIZE) {
    gb_step(gb);
  }
  CHECK(gb_ticks(gb) - before < TO_DOUBLE / 4);
  CHECK(gb_read(gb, IF) & IF_TIMER);
  CHECK(gb_read(gb, KEY1) == KEY1_DOUBLE);

  /* What else a STOP can be. With a button down on a line P1 has selected, and no interrupt waiting, it's a HALT of */
  /* 2 bytes: the INC A after it isn't run. */
  gb_write(gb, TAC, 0);
  gb_write(gb, IE, 0);
  gb_write(gb, P1, P1_BUTTONS);
  gb_buttons(gb, GB_BUTTON_A);
  gb_write(gb, IF, 0);
  run_to(gb, FOURTH_STOP);
  uint8_t a = accumulator(gb);
  gb_step(gb);
  CHECK(pc(gb) == FOURTH_STOP + STOP_SIZE);
  CHECK(halted(gb));
  CHECK(accumulator(gb) == a);

  /* with an interrupt waiting as well it's nothing at all, 1 byte long, and the INC A runs */
  gb_write(gb, IE, IF_TIMER);
  gb_write(gb, IF, IF_TIMER);
  run_to(gb, FIFTH_STOP);
  gb_step(gb);
  CHECK(pc(gb) == FIFTH_STOP + 1);
  CHECK(!halted(gb));
  gb_step(gb);
  CHECK(accumulator(gb) == (uint8_t)(a + 1));
  CHECK(gb_read(gb, KEY1) == KEY1_DOUBLE);

  /* and with no button, KEY1 armed and an interrupt waiting, the speed switches without the CPU standing still: */
  /* 1 byte, and the switch an M-cycle on, as the INC A ends */
  gb_buttons(gb, 0);
  gb_write(gb, KEY1, KEY1_ARMED);
  run_to(gb, SIXTH_STOP);
  CHECK(gb_step(gb) == M_CYCLE / 2);
  CHECK(pc(gb) == SIXTH_STOP + 1);
  CHECK(gb_read(gb, KEY1) == (KEY1_DOUBLE | KEY1_ARMED));
  CHECK(gb_step(gb) == M_CYCLE / 2);
  CHECK(accumulator(gb) == (uint8_t)(a + 2));
  CHECK(gb_read(gb, KEY1) == KEY1_NORMAL);
  CHECK(gb_read(gb, DIV) == 0);

  /* the same going to double speed, with a 2 M-cycle LD A after the STOP: its first M-cycle is at the old speed */
  /* and its second at the new, and the ticks are counted so */
  gb_write(gb, KEY1, KEY1_ARMED);
  run_to(gb, SEVENTH_STOP);
  CHECK(gb_step(gb) == M_CYCLE);
  CHECK(gb_step(gb) == M_CYCLE + M_CYCLE / 2);
  CHECK(gb_read(gb, KEY1) == KEY1_DOUBLE);

  gb_destroy(gb);
  free(rom);
  return test_result("speed");
}
