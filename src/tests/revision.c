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

/* The revision the core settles on for what a host asks for. This one looks inside the core (core.h), where the */
/* others keep to failboy.h. */

#include <stdlib.h>

#include "core.h"
#include "tests/test.h"

/* what the core should be for a host's choice of console, with a color game in or not */
static void test_choice(enum gb_revision asked, int color_game, int expect_revision, int expect_cgb_mode) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, 0, 0);
  if (color_game) {
    rom[TEST_HEADER_CGB] = TEST_HEADER_CGB_GAME;
  }
  struct gb *gb = test_boot(asked, rom, TEST_ROM_SIZE, NULL);
  CHECK(revision == expect_revision);
  CHECK(cgb == ((expect_revision & REVISION_CONSOLE) == REVISION_CGB_0));
  CHECK(cgb_mode == expect_cgb_mode);
  gb_destroy(gb);
  free(rom);
}

int main(void) {
  /* a console's revisions share its high nibble */
  CHECK((REVISION_DMG_C & REVISION_CONSOLE) == REVISION_DMG_0);
  CHECK((REVISION_CGB_E & REVISION_CONSOLE) == REVISION_CGB_0);
  CHECK((REVISION_MGB & REVISION_CONSOLE) != (REVISION_DMG_0 & REVISION_CONSOLE));

  test_choice(GB_AUTO, 0, REVISION_DMG_B, 0);
  test_choice(GB_AUTO, 1, REVISION_CGB_E, 1);
  test_choice(GB_DMG_AUTO, 0, REVISION_DMG_B, 0);
  test_choice(GB_DMG_AUTO, 1, REVISION_DMG_B, 0); /* a color game that also runs on the original */
  test_choice(GB_CGB_AUTO, 1, REVISION_CGB_E, 1);
  test_choice(GB_CGB_AUTO, 0, REVISION_CGB_E, 0); /* compatibility mode */

  /* the consoles it can't be yet */
  static const enum gb_revision unsupported[] = {GB_MGB_AUTO, GB_SGB_AUTO, GB_SGB2_AUTO, GB_AGB_AUTO};

  uint8_t *rom = test_rom(TEST_ROM_SIZE, 0, 0);
  for (size_t i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); ++i) {
    struct gb_config config = {0};
    struct gb *gb = NULL;
    config.revision = unsupported[i];
    config.rom = rom;
    config.rom_size = TEST_ROM_SIZE;
    CHECK(gb_create(&config, &gb) == GB_ERROR_REVISION);
  }
  free(rom);
  return test_result("revision");
}
