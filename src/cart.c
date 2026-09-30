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
#include <string.h>

#include "failboy.h"
#include "files.h"

enum {
  CART_ROM_ONLY = 0,
  CART_MBC1 = 1,
  CART_MBC1_RAM = 2,
  CART_MBC1_RAM_BATT = 3,
  CART_MBC2 = 5,
  CART_MBC2_BATT = 6,
  CART_ROM_RAM = 8,
  CART_ROM_RAM_BATT = 9
};

enum {
  MODE_MODELESS = 0xFF,
  MODE_MBC1_16_8 = 0,
  MODE_MBC1_4_32 = 1,
};

enum {
  HEADER_CART_TYPE = 0x147,
  HEADER_RAM_SIZE = 0x149,
  ROM_BANK_SIZE = 0x4000,
  RAM_BANK_SIZE = 0x2000,
  OPEN_BUS = 0xFF,              /* reading nothing: the data bus floats high */
  MBC1_ROM_BANK_START = 0x2000, /* 0000-1fff enables RAM, 2000-3fff selects the ROM bank */
  MBC1_MODE_START = 0x6000,     /* 4000-5fff selects the RAM bank, 6000-7fff the mode */
  MBC1_ROM_BANK_BITS = 5,
  MBC1_ROM_BANK_MASK = 0x1F,
  MBC1_RAM_BANK_MASK = 0x03,
  MBC1_RAM_ENABLE = 0x0A /* low nibble written to 0000-1fff */
};

/* cart RAM size by header byte 0x149 */
static const unsigned int ram_sizes[6] = {0, 0x800, 0x2000, 0x8000, 0x20000, 0x10000};

static uint8_t *rom;
static uint8_t *ram;

static uint8_t cart_mode = MODE_MODELESS;
static uint8_t rom_bank = 1; /* 2000-3fff: low 5 bits of the ROM bank */
static uint8_t ram_bank = 0; /* 4000-5fff: RAM bank, or bits 5-6 of the ROM bank */
static uint8_t ram_enabled = 0;
static uint8_t battery = 0; /* the RAM keeps its contents with the power off */
static unsigned int rom_size;
static unsigned int rom_mask; /* number of 16 kB banks - 1 */
static unsigned int ram_size;

uint8_t nil_read(uint16_t address) {
  (void)address;
  return OPEN_BUS;
}

void nil_write(uint16_t address, uint8_t value) {
  (void)address;
  (void)value;
}

static read_f ext0_read_f = nil_read; /* 0000-3fff  */
static read_f ext1_read_f = nil_read; /* 4000-7fff */
static read_f ext2_read_f = nil_read; /* a000-bfff */

static write_f ext0_write_f = nil_write; /* 0000-3fff  */
static write_f ext1_write_f = nil_write; /* 4000-7fff */
static write_f ext2_write_f = nil_write; /* a000-bfff */

uint8_t ext0_read(uint16_t address) { return ext0_read_f(address); }

uint8_t ext1_read(uint16_t address) { return ext1_read_f(address); }

uint8_t ext2_read(uint16_t address) { return ext2_read_f(address); }

void ext0_write(uint16_t address, uint8_t value) { ext0_write_f(address, value); }

void ext1_write(uint16_t address, uint8_t value) { ext1_write_f(address, value); }

void ext2_write(uint16_t address, uint8_t value) { ext2_write_f(address, value); }

uint8_t rom_read(uint16_t address) { return rom[address]; }

/* In 4/32 mode the upper bank bits also apply to 0000-3fff (only matters for 1
 * MB+ ROMs). */
uint8_t mbc1_0_read(uint16_t address) {
  unsigned int bank = 0;
  if (cart_mode == MODE_MBC1_4_32) {
    bank = (ram_bank << MBC1_ROM_BANK_BITS) & rom_mask;
  }
  return rom[bank * ROM_BANK_SIZE + address];
}

uint8_t mbc1_1_read(uint16_t address) {
  unsigned int bank = (ram_bank << MBC1_ROM_BANK_BITS | rom_bank) & rom_mask;
  return rom[bank * ROM_BANK_SIZE + address % ROM_BANK_SIZE];
}

void mbc1_0_write(uint16_t address, uint8_t value) {
  /* 0000-3fff  */
  if (address < MBC1_ROM_BANK_START) {
    /* enable disable ram */
    /* TODO not possible on super smart card */
    ram_enabled = (value & 0x0F) == MBC1_RAM_ENABLE;
  } else {
    /* bank 0 can't be selected here */
    rom_bank = value & MBC1_ROM_BANK_MASK;
    if (rom_bank == 0) {
      rom_bank = 1;
    }
  }
}

void mbc1_1_write(uint16_t address, uint8_t value) {
  /* 4000-7fff */
  if (address < MBC1_MODE_START) {
    /* 16/8: rom address lines 19-20, 4/32: ram bank select */
    ram_bank = value & MBC1_RAM_BANK_MASK;
  } else {
    cart_mode = value & 1;
  }
}

static uint8_t *mbc1_ram(uint16_t address) {
  unsigned int bank = 0;
  if (cart_mode == MODE_MBC1_4_32) {
    bank = ram_bank;
  }
  return &ram[(bank * RAM_BANK_SIZE + address % RAM_BANK_SIZE) % ram_size];
}

uint8_t mbc1_2_read(uint16_t address) {
  /* a000-bfff */
  if (!ram_enabled || ram == NULL) {
    return OPEN_BUS;
  }
  return *mbc1_ram(address);
}

void mbc1_2_write(uint16_t address, uint8_t value) {
  /* a000-bfff */
  if (ram_enabled && ram != NULL) {
    *mbc1_ram(address) = value;
  }
}

void cart_mem_reset(void) {
  ext0_read_f = ext1_read_f = ext2_read_f = nil_read;
  ext0_write_f = ext1_write_f = ext2_write_f = nil_write;
  cart_mode = MODE_MODELESS;
  rom_bank = 1;
  ram_bank = 0;
  ram_enabled = 0;
  battery = 0;
  rom_mask = 0;
  ram_size = 0;
  rom = NULL;
  ram = NULL;
}

static int has_battery(uint8_t type) {
  switch (type) {
    case CART_MBC1_RAM_BATT:
    case CART_MBC2_BATT:
    case CART_ROM_RAM_BATT:
      return 1;
    default:
      return 0;
  }
}

int cart_load(const char *filename) {
  if (rom != NULL) {
    cart_free();
  }
  rom = file_load(filename, &rom_size);
  if (rom == NULL) {
    fprintf(stderr, "failboy: can't read '%s'\n", filename);
    return 0;
  }
  /* ROMs come in 32 kB * 2^n */
  if (rom_size < 2 * ROM_BANK_SIZE || (rom_size & (rom_size - 1)) != 0) {
    fprintf(stderr, "failboy: '%s' isn't a Game Boy ROM (size %u)\n", filename, rom_size);
    cart_free();
    return 0;
  }
  rom_mask = rom_size / ROM_BANK_SIZE - 1;

  switch (rom[HEADER_CART_TYPE]) {
    case CART_ROM_ONLY:
      ext0_read_f = ext1_read_f = rom_read;
      break;
    case CART_MBC1:
    case CART_MBC1_RAM:
    case CART_MBC1_RAM_BATT:
      cart_mode = MODE_MBC1_16_8;
      ext0_read_f = mbc1_0_read;
      ext1_read_f = mbc1_1_read;
      ext2_read_f = mbc1_2_read;
      ext0_write_f = mbc1_0_write;
      ext1_write_f = mbc1_1_write;
      ext2_write_f = mbc1_2_write;
      if (rom[HEADER_RAM_SIZE] < sizeof(ram_sizes) / sizeof(ram_sizes[0])) {
        ram_size = ram_sizes[rom[HEADER_RAM_SIZE]];
      }
      break;
    default:
      fprintf(stderr, "failboy: '%s' uses cartridge type %02X, which isn't supported yet\n", filename,
              rom[HEADER_CART_TYPE]);
      cart_free();
      return 0;
  }

  if (ram_size) {
    ram = calloc(1, ram_size);
    if (ram == NULL) {
      fprintf(stderr, "failboy: out of memory\n");
      cart_free();
      return 0;
    }
  }
  battery = has_battery(rom[HEADER_CART_TYPE]);
  return 1;
}

unsigned int cart_battery_ram_size(void) {
  if (!battery) {
    return 0;
  }
  return ram_size;
}

void cart_read_battery_ram(uint8_t *buffer, unsigned int size) {
  if (size > cart_battery_ram_size()) {
    size = cart_battery_ram_size();
  }
  memcpy(buffer, ram, size);
}

void cart_write_battery_ram(const uint8_t *buffer, unsigned int size) {
  if (size > cart_battery_ram_size()) {
    size = cart_battery_ram_size();
  }
  memcpy(ram, buffer, size);
}

void cart_free(void) {
  if (rom != NULL) {
    free(rom);
  }
  if (ram != NULL) {
    free(ram);
  }
  cart_mem_reset();
}
