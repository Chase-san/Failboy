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

/* The timer: TIMA counts the falling edges of one bit of the divider, so anything that makes that bit fall counts, */
/* a write to DIV or TAC included. The cartridge is all NOPs, each 4 ticks. */

#include <stdlib.h>

#include "tests/test.h"

enum {
  DIV = 0xFF04,
  TIMA = 0xFF05,
  TMA = 0xFF06,
  TAC = 0xFF07,
  IF = 0xFF0F,
  IF_TIMER = 0x04,
  TAC_OFF = 0x01,     /* the fastest clock selected, but stopped */
  TAC_FASTEST = 0x05, /* divider bit 3: a count every 16 ticks */
  TAC_SLOWEST = 0x04, /* divider bit 9: every 1024 */
  FASTEST_TICKS = 16,
  SLOWEST_TICKS = 1024,
  NOP_TICKS = 4,
  DIV_AFTER_BOOT = 0xAB, /* the original Game Boy's; the divider's low byte is 0xCC, with bit 3 set */
};

static void nops(struct gb *gb, int count) {
  for (int i = 0; i < count; ++i) {
    CHECK(gb_step(gb) == NOP_TICKS);
  }
}

int main(void) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, 0, 0);
  struct gb *gb = test_boot(GB_DMG_AUTO, rom, TEST_ROM_SIZE, NULL);
  CHECK(gb_read(gb, DIV) == DIV_AFTER_BOOT);
  CHECK(gb_read(gb, TIMA) == 0);

  /* resetting the divider while the selected bit is high makes it fall */
  gb_write(gb, TAC, TAC_FASTEST);
  gb_write(gb, DIV, 0x55); /* whatever's written, it's a reset */
  CHECK(gb_read(gb, DIV) == 0);
  CHECK(gb_read(gb, TIMA) == 1);
  /* and while it's low, doesn't */
  gb_write(gb, DIV, 0);
  CHECK(gb_read(gb, TIMA) == 1);

  /* so does stopping the timer, which gates the bit off */
  nops(gb, 2); /* the divider at 8: bit 3 high */
  gb_write(gb, TAC, TAC_OFF);
  CHECK(gb_read(gb, TIMA) == 2);
  /* stopped, it doesn't count */
  nops(gb, FASTEST_TICKS / NOP_TICKS * 4);
  CHECK(gb_read(gb, TIMA) == 2);

  /* its rates */
  gb_write(gb, DIV, 0);
  gb_write(gb, TIMA, 0);
  gb_write(gb, TAC, TAC_FASTEST);
  nops(gb, FASTEST_TICKS / NOP_TICKS * 10);
  CHECK(gb_read(gb, TIMA) == 10);
  gb_write(gb, TAC, TAC_SLOWEST); /* bit 9 is low, and bit 3 has just fallen: no count */
  gb_write(gb, DIV, 0);
  gb_write(gb, TIMA, 0);
  nops(gb, SLOWEST_TICKS / NOP_TICKS * 3 - 1);
  CHECK(gb_read(gb, TIMA) == 2);
  nops(gb, 1);
  CHECK(gb_read(gb, TIMA) == 3);
  CHECK(gb_read(gb, DIV) == SLOWEST_TICKS * 3 >> 8);

  /* Past FF it starts again from TMA and asks for its interrupt. On the real thing that's an M-cycle late, so the */
  /* check waits one out to hold either way. */
  gb_write(gb, TAC, TAC_FASTEST);
  gb_write(gb, DIV, 0);
  gb_write(gb, TMA, 0x42);
  gb_write(gb, TIMA, 0xFF);
  gb_write(gb, IF, 0);
  nops(gb, FASTEST_TICKS / NOP_TICKS - 1);
  CHECK(gb_read(gb, TIMA) == 0xFF);
  CHECK(!(gb_read(gb, IF) & IF_TIMER));
  nops(gb, 2);
  CHECK(gb_read(gb, TIMA) == 0x42);
  CHECK(gb_read(gb, IF) & IF_TIMER);

  gb_destroy(gb);
  free(rom);
  return test_result("timer");
}
