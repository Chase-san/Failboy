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
#include "rtc.h"

/* cartridge types, from header byte 0x147 */
enum {
  CART_ROM_ONLY = 0x00,
  CART_MBC1 = 0x01,
  CART_MBC1_RAM = 0x02,
  CART_MBC1_RAM_BATT = 0x03,
  CART_MBC2 = 0x05,
  CART_MBC2_BATT = 0x06,
  CART_ROM_RAM = 0x08,
  CART_ROM_RAM_BATT = 0x09,
  CART_MBC3_TIMER_BATT = 0x0F,
  CART_MBC3_TIMER_RAM_BATT = 0x10,
  CART_MBC3 = 0x11,
  CART_MBC3_RAM = 0x12,
  CART_MBC3_RAM_BATT = 0x13,
  CART_MBC5 = 0x19,
  CART_MBC5_RAM = 0x1A,
  CART_MBC5_RAM_BATT = 0x1B,
  CART_MBC5_RUMBLE = 0x1C,
  CART_MBC5_RUMBLE_RAM = 0x1D,
  CART_MBC5_RUMBLE_RAM_BATT = 0x1E,
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
  OPEN_BUS = 0xFF,   /* reading nothing: the data bus floats high */
  RAM_ENABLE = 0x0A, /* in the low nibble of a write to 0000-1fff, on every MBC */
  RAM_ENABLE_MASK = 0x0F,
  ROM_BANK_START = 0x2000, /* 0000-1fff enables RAM, 2000-3fff selects the ROM bank */
  UPPER_START = 0x6000,    /* 4000-5fff selects the RAM bank, 6000-7fff the mode (MBC1) or latches the clock (MBC3) */
};

enum {
  MBC1_ROM_BANK_BITS = 5,
  MBC1_ROM_BANK_MASK = 0x1F,
  MBC1_RAM_BANK_MASK = 0x03,
};

/* MBC2: 512 half-bytes of RAM built in, and its two registers told apart by address bit 8 */
enum {
  MBC2_RAM_SIZE = 0x200,
  MBC2_RAM_MASK = 0x0F,
  MBC2_RAM_UNUSED = 0xF0, /* the missing top halves */
  MBC2_ROM_BANK_SELECT = 0x0100,
  MBC2_ROM_BANK_MASK = 0x0F,
};

/* MBC3, and the MBC30 (8-bit ROM bank, 8 RAM banks): 4000-5fff picks a RAM bank or one of the clock's registers */
enum {
  MBC3_RAM_BANK_MASK = 0x07,
  MBC3_RTC_FIRST = 0x08,
  MBC3_RTC_LAST = 0x0C,
  MBC3_LATCH_ARM = 0x00, /* writing 0 and then 1 to 6000-7fff latches the clock */
  MBC3_LATCH = 0x01,
};

/* MBC5: a 9-bit ROM bank, where bank 0 is allowed */
enum {
  MBC5_ROM_BANK_HIGH_START = 0x3000, /* 2000-2fff: bits 0-7 of the ROM bank, 3000-3fff: bit 8 */
  MBC5_ROM_BANK_LOW_MASK = 0xFF,
  MBC5_ROM_BANK_HIGH_SHIFT = 8,
  MBC5_RAM_BANK_MASK = 0x0F,
  MBC5_RUMBLE = 0x08, /* on rumble carts this bit of the RAM bank runs the motor instead */
};

/* cart RAM size by header byte 0x149 */
static const unsigned int ram_sizes[6] = {0, 0x800, 0x2000, 0x8000, 0x20000, 0x10000};

static uint8_t *rom;
static uint8_t *ram;

static uint8_t cart_mode = MODE_MODELESS;
static unsigned int rom_bank = 1; /* 2000-3fff (MBC1: its low 5 bits) */
static uint8_t ram_bank = 0; /* 4000-5fff: RAM bank; MBC1: or bits 5-6 of the ROM bank; MBC3: or a clock register */
static uint8_t ram_enabled = 0;
static uint8_t latch_armed = 0; /* MBC3: 0 was just written to 6000-7fff */
static uint8_t rumble = 0;
static uint8_t battery = 0; /* the RAM keeps its contents with the power off */
static uint8_t rtc = 0;     /* MBC3 with a clock */
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

/* **************************************** */
/* Shared by the MBCs */

static uint8_t rom_at(unsigned int bank, uint16_t address) {
  return rom[(bank & rom_mask) * ROM_BANK_SIZE + address % ROM_BANK_SIZE];
}

static uint8_t *ram_at(unsigned int bank, uint16_t address) {
  return &ram[(bank * RAM_BANK_SIZE + address % RAM_BANK_SIZE) % ram_size];
}

static uint8_t ram_enable(uint8_t value) { return (value & RAM_ENABLE_MASK) == RAM_ENABLE; }

/* 0000-3fff (all of it, for a ROM without an MBC) */
uint8_t rom_read(uint16_t address) { return rom[address]; }

/* 4000-7fff, for the MBC2, MBC3 and MBC5 */
uint8_t banked_rom_read(uint16_t address) { return rom_at(rom_bank, address); }

/* a000-bfff, for the MBC5 */
uint8_t banked_ram_read(uint16_t address) {
  if (!ram_enabled || ram == NULL) {
    return OPEN_BUS;
  }
  return *ram_at(ram_bank, address);
}

void banked_ram_write(uint16_t address, uint8_t value) {
  if (ram_enabled && ram != NULL) {
    *ram_at(ram_bank, address) = value;
  }
}

/* **************************************** */
/* MBC1 */

/* In 4/32 mode the upper bank bits also apply to 0000-3fff (only matters for 1 MB+ ROMs). */
uint8_t mbc1_0_read(uint16_t address) {
  unsigned int bank = 0;
  if (cart_mode == MODE_MBC1_4_32) {
    bank = ram_bank << MBC1_ROM_BANK_BITS;
  }
  return rom_at(bank, address);
}

uint8_t mbc1_1_read(uint16_t address) { return rom_at(ram_bank << MBC1_ROM_BANK_BITS | rom_bank, address); }

void mbc1_0_write(uint16_t address, uint8_t value) {
  /* 0000-3fff  */
  if (address < ROM_BANK_START) {
    /* enable disable ram */
    /* TODO not possible on super smart card */
    ram_enabled = ram_enable(value);
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
  if (address < UPPER_START) {
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
  return ram_at(bank, address);
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

/* **************************************** */
/* MBC2 */

void mbc2_0_write(uint16_t address, uint8_t value) {
  /* 0000-3fff: address bit 8 picks the register */
  if (address & MBC2_ROM_BANK_SELECT) {
    rom_bank = value & MBC2_ROM_BANK_MASK;
    if (rom_bank == 0) {
      rom_bank = 1;
    }
  } else {
    ram_enabled = ram_enable(value);
  }
}

uint8_t mbc2_2_read(uint16_t address) {
  /* a000-bfff: the 512 half-bytes, over and over */
  if (!ram_enabled) {
    return OPEN_BUS;
  }
  return ram[address % MBC2_RAM_SIZE] | MBC2_RAM_UNUSED;
}

void mbc2_2_write(uint16_t address, uint8_t value) {
  if (ram_enabled) {
    ram[address % MBC2_RAM_SIZE] = value & MBC2_RAM_MASK;
  }
}

/* **************************************** */
/* MBC3 and MBC30 */

void mbc3_0_write(uint16_t address, uint8_t value) {
  /* 0000-3fff */
  if (address < ROM_BANK_START) {
    /* the RAM and the clock */
    ram_enabled = ram_enable(value);
  } else {
    /* 7 bits on the MBC3, 8 on the MBC30; the ROM's size trims off the rest */
    rom_bank = value;
    if (rom_bank == 0) {
      rom_bank = 1;
    }
  }
}

void mbc3_1_write(uint16_t address, uint8_t value) {
  /* 4000-7fff */
  if (address < UPPER_START) {
    ram_bank = value;
  } else {
    if (rtc && latch_armed && value == MBC3_LATCH) {
      rtc_latch();
    }
    latch_armed = value == MBC3_LATCH_ARM;
  }
}

static int mbc3_clock_selected(void) { return ram_bank >= MBC3_RTC_FIRST; }

uint8_t mbc3_2_read(uint16_t address) {
  /* a000-bfff: a RAM bank or a clock register */
  if (!ram_enabled) {
    return OPEN_BUS;
  }
  if (mbc3_clock_selected()) {
    if (rtc && ram_bank <= MBC3_RTC_LAST) {
      return rtc_read(ram_bank - MBC3_RTC_FIRST);
    }
    return OPEN_BUS;
  }
  if (ram == NULL) {
    return OPEN_BUS;
  }
  return *ram_at(ram_bank & MBC3_RAM_BANK_MASK, address);
}

void mbc3_2_write(uint16_t address, uint8_t value) {
  if (!ram_enabled) {
    return;
  }
  if (mbc3_clock_selected()) {
    if (rtc && ram_bank <= MBC3_RTC_LAST) {
      rtc_write(ram_bank - MBC3_RTC_FIRST, value);
    }
    return;
  }
  if (ram != NULL) {
    *ram_at(ram_bank & MBC3_RAM_BANK_MASK, address) = value;
  }
}

/* **************************************** */
/* MBC5 */

void mbc5_0_write(uint16_t address, uint8_t value) {
  /* 0000-3fff */
  if (address < ROM_BANK_START) {
    ram_enabled = ram_enable(value);
  } else if (address < MBC5_ROM_BANK_HIGH_START) {
    rom_bank = (rom_bank & ~MBC5_ROM_BANK_LOW_MASK) | value;
  } else {
    rom_bank = (rom_bank & MBC5_ROM_BANK_LOW_MASK) | (value & 1) << MBC5_ROM_BANK_HIGH_SHIFT;
  }
}

void mbc5_1_write(uint16_t address, uint8_t value) {
  /* 4000-7fff */
  if (address < UPPER_START) {
    ram_bank = value & MBC5_RAM_BANK_MASK;
    if (rumble) {
      ram_bank &= ~MBC5_RUMBLE;
    }
  }
}

/* **************************************** */
/* Loading */

void cart_mem_reset(void) {
  ext0_read_f = ext1_read_f = ext2_read_f = nil_read;
  ext0_write_f = ext1_write_f = ext2_write_f = nil_write;
  cart_mode = MODE_MODELESS;
  rom_bank = 1;
  ram_bank = 0;
  ram_enabled = 0;
  latch_armed = 0;
  rumble = 0;
  battery = 0;
  rtc = 0;
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
    case CART_MBC3_TIMER_BATT:
    case CART_MBC3_TIMER_RAM_BATT:
    case CART_MBC3_RAM_BATT:
    case CART_MBC5_RAM_BATT:
    case CART_MBC5_RUMBLE_RAM_BATT:
      return 1;
    default:
      return 0;
  }
}

static int has_rtc(uint8_t type) { return type == CART_MBC3_TIMER_BATT || type == CART_MBC3_TIMER_RAM_BATT; }

static int has_rumble(uint8_t type) { return type >= CART_MBC5_RUMBLE && type <= CART_MBC5_RUMBLE_RAM_BATT; }

static unsigned int header_ram_size(void) {
  if (rom[HEADER_RAM_SIZE] < sizeof(ram_sizes) / sizeof(ram_sizes[0])) {
    return ram_sizes[rom[HEADER_RAM_SIZE]];
  }
  return 0;
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

  uint8_t type = rom[HEADER_CART_TYPE];
  switch (type) {
    case CART_ROM_ONLY:
      ext0_read_f = ext1_read_f = rom_read;
      break;
    case CART_ROM_RAM:
    case CART_ROM_RAM_BATT:
      /* no MBC to switch the RAM on and off, so it's always on */
      ext0_read_f = ext1_read_f = rom_read;
      ext2_read_f = banked_ram_read;
      ext2_write_f = banked_ram_write;
      ram_size = header_ram_size();
      ram_enabled = 1;
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
      ram_size = header_ram_size();
      break;
    case CART_MBC2:
    case CART_MBC2_BATT:
      ext0_read_f = rom_read;
      ext1_read_f = banked_rom_read;
      ext2_read_f = mbc2_2_read;
      ext0_write_f = mbc2_0_write;
      ext2_write_f = mbc2_2_write;
      ram_size = MBC2_RAM_SIZE;
      break;
    case CART_MBC3_TIMER_BATT:
    case CART_MBC3_TIMER_RAM_BATT:
    case CART_MBC3:
    case CART_MBC3_RAM:
    case CART_MBC3_RAM_BATT:
      ext0_read_f = rom_read;
      ext1_read_f = banked_rom_read;
      ext2_read_f = mbc3_2_read;
      ext0_write_f = mbc3_0_write;
      ext1_write_f = mbc3_1_write;
      ext2_write_f = mbc3_2_write;
      ram_size = header_ram_size();
      break;
    case CART_MBC5:
    case CART_MBC5_RAM:
    case CART_MBC5_RAM_BATT:
    case CART_MBC5_RUMBLE:
    case CART_MBC5_RUMBLE_RAM:
    case CART_MBC5_RUMBLE_RAM_BATT:
      ext0_read_f = rom_read;
      ext1_read_f = banked_rom_read;
      ext2_read_f = banked_ram_read;
      ext0_write_f = mbc5_0_write;
      ext1_write_f = mbc5_1_write;
      ext2_write_f = banked_ram_write;
      ram_size = header_ram_size();
      break;
    default:
      fprintf(stderr, "failboy: '%s' uses cartridge type %02X, which isn't supported yet\n", filename, type);
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
  battery = has_battery(type);
  rumble = has_rumble(type);
  rtc = has_rtc(type);
  if (rtc) {
    rtc_reset();
  }
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

_Static_assert(CART_RTC_SIZE == RTC_STATE_SIZE, "the clock's saved state changed size");

int cart_has_rtc(void) { return rtc; }

void cart_read_rtc(uint8_t *buffer, unsigned int size) {
  if (rtc && size >= CART_RTC_SIZE) {
    rtc_save(buffer);
  }
}

void cart_write_rtc(const uint8_t *buffer, unsigned int size) {
  if (rtc && size >= CART_RTC_SIZE) {
    rtc_load(buffer);
  }
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
