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

/* The mappers' bank switching: which bank of the ROM or RAM the CPU ends up looking at. */

#include <stdlib.h>

#include "tests/test.h"

enum {
  FIXED = 0x0000,    /* the ROM's first half, bank 0 unless the mapper says otherwise */
  SWITCHED = 0x4000, /* its second, the bank selected */
  RAM = 0xA000,
  RAM_ENABLE = 0x0000, /* the mappers' registers, by where they're written */
  ROM_BANK = 0x2000,
  MBC2_ROM_BANK = 0x2100, /* address bit 8 set */
  MBC5_ROM_BANK_HIGH = 0x3000,
  UPPER_BANK = 0x4000,
  MBC1_MODE = 0x6000,
  ENABLE = 0x0A,
  OPEN_BUS = 0xFF,
  CART_MBC1 = 0x01,
  CART_MBC1_RAM = 0x02,
  CART_MBC2 = 0x05,
  CART_MBC5 = 0x19,
  RAM_32K = 0x03, /* the header's code for it */
  RAM_BANK_SIZE = 0x2000,
  ROM_128K = 8 * TEST_BANK_SIZE,
  ROM_1M = 64 * TEST_BANK_SIZE,
  ROM_8M = 512 * TEST_BANK_SIZE,
  LOGO = 0x104, /* where a game's header has the logo, by which a multicart's games are found */
  LOGO_SIZE = 0x30,
  MULTICART_GAME = 16 * TEST_BANK_SIZE, /* a multicart's games are this far apart */
};

static void test_mbc1_rom(void) {
  uint8_t *rom = test_rom(ROM_1M, CART_MBC1, 0);
  struct gb *gb = test_boot(GB_DMG_AUTO, rom, ROM_1M, NULL);
  CHECK(test_bank(gb, FIXED) == 0 && test_bank(gb, SWITCHED) == 1);
  gb_write(gb, ROM_BANK, 0x05);
  CHECK(test_bank(gb, SWITCHED) == 0x05);
  /* bank 0 can't be selected: it gives 1 */
  gb_write(gb, ROM_BANK, 0x00);
  CHECK(test_bank(gb, SWITCHED) == 0x01);
  /* only 5 bits of it, so 0x20 is 0 too */
  gb_write(gb, ROM_BANK, 0x20);
  CHECK(test_bank(gb, SWITCHED) == 0x01);
  gb_write(gb, ROM_BANK, 0xFF);
  CHECK(test_bank(gb, SWITCHED) == 0x1F);
  /* the upper register gives 2 more bits, with which the 0-to-1 rule lands on 0x21 */
  gb_write(gb, UPPER_BANK, 0x01);
  CHECK(test_bank(gb, SWITCHED) == 0x3F);
  gb_write(gb, ROM_BANK, 0x00);
  CHECK(test_bank(gb, SWITCHED) == 0x21);
  /* and in the other mode they apply to the first half as well */
  CHECK(test_bank(gb, FIXED) == 0);
  gb_write(gb, MBC1_MODE, 0x01);
  CHECK(test_bank(gb, FIXED) == 0x20);
  gb_write(gb, MBC1_MODE, 0x00);
  CHECK(test_bank(gb, FIXED) == 0);
  gb_destroy(gb);
  free(rom);

  /* bits the ROM is too small for are left off */
  rom = test_rom(ROM_128K, CART_MBC1, 0);
  gb = test_boot(GB_DMG_AUTO, rom, ROM_128K, NULL);
  gb_write(gb, ROM_BANK, 0x0F);
  CHECK(test_bank(gb, SWITCHED) == 0x07);
  gb_write(gb, ROM_BANK, 0x08);
  CHECK(test_bank(gb, SWITCHED) == 0x00);
  gb_destroy(gb);
  free(rom);
}

static void test_mbc1_ram(void) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, CART_MBC1_RAM, RAM_32K);
  struct gb *gb = test_boot(GB_DMG_AUTO, rom, TEST_ROM_SIZE, NULL);
  uint32_t size = 0;
  uint8_t *ram = gb_memory(gb, GB_MEMORY_CART_RAM, &size);
  CHECK(ram != NULL && size == 4 * RAM_BANK_SIZE);
  CHECK(gb_read(gb, RAM) == OPEN_BUS);
  gb_write(gb, RAM, 0x11); /* while it's off: lost */
  gb_write(gb, RAM_ENABLE, ENABLE);
  CHECK(gb_read(gb, RAM) == 0x00);
  gb_write(gb, RAM, 0x22);
  /* the upper register only picks a RAM bank in the other mode */
  gb_write(gb, UPPER_BANK, 0x02);
  CHECK(gb_read(gb, RAM) == 0x22);
  gb_write(gb, MBC1_MODE, 0x01);
  CHECK(gb_read(gb, RAM) == 0x00);
  gb_write(gb, RAM, 0x33);
  CHECK(ram[0] == 0x22 && ram[2 * RAM_BANK_SIZE] == 0x33);
  /* anything but 0x0A in the low nibble switches it off */
  gb_write(gb, RAM_ENABLE, 0x0B);
  CHECK(gb_read(gb, RAM) == OPEN_BUS);
  gb_write(gb, RAM_ENABLE, 0xFA);
  CHECK(gb_read(gb, RAM) == 0x33);
  gb_destroy(gb);
  free(rom);
}

static void test_mbc2(void) {
  uint8_t *rom = test_rom(ROM_128K, CART_MBC2, 0);
  struct gb *gb = test_boot(GB_DMG_AUTO, rom, ROM_128K, NULL);
  /* one register, told apart by address bit 8 */
  gb_write(gb, MBC2_ROM_BANK, 0x03);
  CHECK(test_bank(gb, SWITCHED) == 0x03);
  gb_write(gb, MBC2_ROM_BANK, 0x00);
  CHECK(test_bank(gb, SWITCHED) == 0x01);
  CHECK(gb_read(gb, RAM) == OPEN_BUS);
  gb_write(gb, RAM_ENABLE, ENABLE);
  CHECK(test_bank(gb, SWITCHED) == 0x01);
  /* half-bytes of RAM: the top halves read as 1s */
  gb_write(gb, RAM, 0xA5);
  CHECK(gb_read(gb, RAM) == 0xF5);
  /* and only 512 of them, over and over */
  CHECK(gb_read(gb, RAM + 0x200) == 0xF5);
  gb_destroy(gb);
  free(rom);
}

static void test_mbc5(void) {
  uint8_t *rom = test_rom(ROM_8M, CART_MBC5, 0);
  struct gb *gb = test_boot(GB_DMG_AUTO, rom, ROM_8M, NULL);
  CHECK(test_bank(gb, SWITCHED) == 1);
  /* bank 0 can be selected here */
  gb_write(gb, ROM_BANK, 0x00);
  CHECK(test_bank(gb, SWITCHED) == 0);
  gb_write(gb, ROM_BANK, 0xFF);
  CHECK(test_bank(gb, SWITCHED) == 0xFF);
  /* a ninth bit */
  gb_write(gb, MBC5_ROM_BANK_HIGH, 0x01);
  CHECK(test_bank(gb, SWITCHED) == 0x1FF);
  gb_write(gb, ROM_BANK, 0x00);
  CHECK(test_bank(gb, SWITCHED) == 0x100);
  gb_write(gb, MBC5_ROM_BANK_HIGH, 0xFE); /* only bit 0 of it */
  CHECK(test_bank(gb, SWITCHED) == 0x000);
  CHECK(test_bank(gb, FIXED) == 0);
  gb_destroy(gb);
  free(rom);
}

/* A multicart: four games behind one MBC1, each with its header, the upper register picking the game and only 4 */
/* bits of the lower one reaching the ROM. */
static void test_mbc1_multicart(void) {
  uint8_t *rom = test_rom(ROM_1M, CART_MBC1, 0);
  for (int game = 0; game < 4; ++game) {
    for (int i = 0; i < LOGO_SIZE; ++i) {
      rom[game * MULTICART_GAME + LOGO + i] = (uint8_t)(i + 1);
    }
  }
  struct gb *gb = test_boot(GB_DMG_AUTO, rom, ROM_1M, NULL);
  CHECK(test_bank(gb, FIXED) == 0 && test_bank(gb, SWITCHED) == 1);
  gb_write(gb, UPPER_BANK, 0x01);
  CHECK(test_bank(gb, SWITCHED) == 0x11);
  gb_write(gb, ROM_BANK, 0x05);
  CHECK(test_bank(gb, SWITCHED) == 0x15);
  /* the lower register's fifth bit doesn't reach the ROM, but still keeps it from being 0: the game's first bank */
  gb_write(gb, ROM_BANK, 0x10);
  CHECK(test_bank(gb, SWITCHED) == 0x10);
  gb_write(gb, ROM_BANK, 0x00);
  CHECK(test_bank(gb, SWITCHED) == 0x11);
  /* in the other mode the game's first bank is at the bottom too */
  gb_write(gb, MBC1_MODE, 0x01);
  CHECK(test_bank(gb, FIXED) == 0x10);
  gb_write(gb, UPPER_BANK, 0x03);
  CHECK(test_bank(gb, FIXED) == 0x30 && test_bank(gb, SWITCHED) == 0x31);
  gb_destroy(gb);
  free(rom);
}

int main(void) {
  test_mbc1_rom();
  test_mbc1_multicart();
  test_mbc1_ram();
  test_mbc2();
  test_mbc5();
  return test_result("cart");
}
