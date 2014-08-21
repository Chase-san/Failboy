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
#include "failboy.h"
#include "files.h"
#include <assert.h>

#define assert2(x) assert(!(x))

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
	MODE_MBC1_4_32 = 1
};

static uint8_t *rom;
static uint8_t *ram;

static uint8_t cart_mode = MODE_MODELESS;
static uint8_t rom_bank = 1;
static uint8_t ram_bank = 0;
static unsigned int size;

uint8_t nil_read(uint16_t address) { return 0; }
void nil_write(uint16_t address, uint8_t value) { }

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

uint8_t rom_read(uint16_t address) {
	return rom[address];
}

uint8_t mbc1_1_read(uint16_t address) {
	return rom[rom_bank * 0x4000 + address - 0x4000];
}

void mbc1_0_write(uint16_t address, uint8_t value) {
	/* 0000-3fff  */
	if(address < 0x2000) {
		/* enable disable ram */
		/* TODO not possible on super smart card */
		if((value & 0xF) == 0xA) {
			/* TODO enable ram */
		} else {
			/* TODO disable ram */
		}
	} else {
		if(value == 0) {
			value = 1;
		}
		rom_bank = value;
	}
}

void mbc1_1_write(uint16_t address, uint8_t value) {
	/* 4000-7fff */
	if(address < 0x6000) {
		if(cart_mode == MODE_MBC1_16_8) {
			/* 16/8  rom address lines */
		} else {
			/* 4/32  ram bank select */
			ram_bank = value & 3;
		}
	} else {
		cart_mode = value & 1;
		if(cart_mode == MODE_MBC1_16_8) {
			ram_bank = 0;
		}
	}
}

void cart_mem_reset() {
	ext0_read_f = ext1_read_f = ext2_read_f = nil_read;
	ext0_write_f = ext1_write_f = ext2_write_f = nil_write;
	rom_bank = 1;
	rom = NULL;
	ram = NULL;
}

void cart_load(const char *filename) {
	if(rom != NULL) {
		cart_free();
	}
	rom = sb_file_load2(filename, &size);
	assert2(rom == NULL);
	switch(rom[0x147]) {
		case CART_ROM_ONLY:
			ext0_read_f = ext1_read_f = rom_read;
			break;
		case CART_MBC1:
			cart_mode = MODE_MBC1_16_8;
			ext0_read_f = rom_read;
			ext1_read_f = mbc1_1_read;
			ext0_write_f = mbc1_0_write;
			ext1_write_f = mbc1_1_write;
			break;
		default:
			break;
	}
}

void cart_free() {
	if(rom != NULL) {
		free(rom);
	}
	if(ram != NULL) {
		free(ram);
	}
	cart_mem_reset();
}
