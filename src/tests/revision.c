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

/* The hardware the core settles on for what a host asks for, as the host is told it (gb_hardware()) and as the */
/* modules see it. This one looks inside the core (core.h), where the others keep to failboy.h. */

#include <stdlib.h>

#include "core.h"
#include "tests/test.h"

/* what the core should be for a host's choice of console, with a color game in or not */
static void test_choice(enum gb_revision asked, int color_game, enum gb_hardware expect, int expect_cgb_mode) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, 0, 0);
  if (color_game) {
    rom[TEST_HEADER_CGB] = TEST_HEADER_CGB_GAME;
  }
  struct gb *gb = test_boot(asked, rom, TEST_ROM_SIZE, NULL);
  CHECK(gb_hardware(gb) == expect);
  CHECK(revision == (int)expect);
  CHECK(cgb == ((expect & GB_CONSOLE) == GB_CONSOLE_CGB));
  CHECK(cgb_mode == expect_cgb_mode);
  if (cgb) {
    CHECK(gb_pixel_format(gb) == GB_PIXELS_COLORS);
  } else {
    CHECK(gb_pixel_format(gb) == GB_PIXELS_SHADES);
  }
  gb_destroy(gb);
  free(rom);
}

int main(void) {
  /* the console is the high nibble */
  CHECK((GB_HARDWARE_DMG_B & GB_CONSOLE) == GB_CONSOLE_DMG);
  CHECK((GB_HARDWARE_CGB_E & GB_CONSOLE) == GB_CONSOLE_CGB);

  test_choice(GB_AUTO, 0, GB_HARDWARE_DMG_B, 0);
  test_choice(GB_AUTO, 1, GB_HARDWARE_CGB_E, 1);
  test_choice(GB_DMG_AUTO, 0, GB_HARDWARE_DMG_B, 0);
  test_choice(GB_DMG_AUTO, 1, GB_HARDWARE_DMG_B, 0); /* a color game that also runs on the original */
  test_choice(GB_CGB_AUTO, 1, GB_HARDWARE_CGB_E, 1);
  test_choice(GB_CGB_AUTO, 0, GB_HARDWARE_CGB_E, 0); /* compatibility mode */
  return test_result("revision");
}
