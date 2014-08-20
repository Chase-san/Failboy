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
#include <stdlib.h>

/* ************************************************************** */
/* cart.c */
uint8_t ext0_read(uint16_t); /* 0000-3FFF */
uint8_t ext1_read(uint16_t); /* 4000-7FFF */
uint8_t ext2_read(uint16_t); /* A000-BFFF */

void ext0_write(uint16_t, uint8_t); /* 0000-3FFF */
void ext1_write(uint16_t, uint8_t); /* 4000-7FFF */
void ext2_write(uint16_t, uint8_t); /* A000-BFFF */

/* video.c */
uint8_t vram_read(uint16_t); /* 8000-9FFF */
void vram_write(uint16_t, uint8_t); /* 8000-9FFF */

/* I use function pointers here to make things easier, faster and simpler. Built in bound checking is nice as well. */

/* ************************************************************** */

static uint8_t *wram;
static uint8_t *hram;
uint8_t *oam;
uint8_t *vram;

void mem_alloc() {
	/* 8 kB Working Ram */
	wram = malloc(0x2000);
	hram = malloc(127);
	oam = malloc(160);
	vram = malloc(0x2000);
}

void mem_free() {
	free(vram);
	free(oam);
	free(hram);
	free(wram);
}

/* ************************************************************** */
/* READ */
uint8_t wram_read(uint16_t); /* C000-DFFF */
uint8_t wrame_read(uint16_t); /* E000-FDFF */
uint8_t oam_read(uint16_t); /* FE00-FE9F */
uint8_t fxxx_read(uint16_t); /* F000-FFFF */
uint8_t cpu_read(uint16_t); /* FF00-FFFF */
uint8_t hram_read(uint16_t); /* FF80-FFFE */
uint8_t io_read(uint16_t); /* FF00-FF7F */


/* memory map */
static const read_f readmap[16] = {
	/* 0000-3fff  external cart 0 */
	ext0_read, ext0_read, ext0_read, ext0_read,
	/* 4000-7fff  external cart 0 */
	ext1_read, ext1_read, ext1_read, ext1_read,
	/* 8000-9fff  8kB Video Ram */
	vram_read, vram_read,
	/* a000-bfff  external cart stuff */
	ext2_read, ext2_read,
	/* c000-dfff  8kB Work Ram */
	wram_read, wram_read,
	/* e000-fdff  Work Ram Echo (usually unused) */
	wrame_read, /* e000 */
	/* fe00-ffff  CPU stuff */
	fxxx_read /* f000 */
};

/* fxxx range */
static const read_f fxxx_readmap[16] = {
	/* 000h-dffh  Work Ram Echo */
	wrame_read, wrame_read, wrame_read, wrame_read,
	wrame_read, wrame_read, wrame_read, wrame_read,
	wrame_read, wrame_read, wrame_read, wrame_read,
	wrame_read, wrame_read,
	/* e00-e9f  OAM (160 bytes) */
	/* ea0-eff  NIL */
	oam_read,
	/* f00-fff  CPU */
	cpu_read
};

uint8_t wram_read(uint16_t address) {
	return wram[address - 0xC000];
}

uint8_t wrame_read(uint16_t address) {
	return wram_read(address - 0x2000);
}

uint8_t oam_read(uint16_t address) {
	if(address < 0xfea0) {
		return oam[address - 0xfe00];
	}
	return 0;
}

uint8_t fxxx_read(uint16_t address) {
	return fxxx_readmap[(address >> 8) & 0xF](address);
}

uint8_t cpu_read(uint16_t address) {
	if(address == 0xFFFF) {
		return io_read(address);
	}
	if((address >> 7) & 1) {
		return hram_read(address);
	}
	return io_read(address);
}

uint8_t hram_read(uint16_t address) {
	return hram[(uint8_t)address - 0x80];
}

uint8_t io_read(uint16_t address) {
	return 0;
}

uint8_t read(uint16_t address) {
	/* Use address blocks to avoid if branching. :D */
	/* With just 16 blocks we can massively reduce the branching here. */
	return readmap[address >> 12](address);
}

uint16_t read16(uint16_t address) {
	return (read(address)) | (read(address + 1) << 8);
}


/* ************************************************************** */
/* WRITE */
void wram_write(uint16_t, uint8_t); /* C000-DFFF */
void wrame_write(uint16_t, uint8_t); /* E000-FDFF */
void oam_write(uint16_t, uint8_t); /* FE00-FE9F */
void fxxx_write(uint16_t, uint8_t); /* F000-FFFF */
void cpu_write(uint16_t, uint8_t); /* FF00-FFFF */
void io_write(uint16_t, uint8_t); /* FF80-FFFE */
void hram_write(uint16_t, uint8_t); /* FF00-FF7F */
void vram_write(uint16_t, uint8_t); /* 8000-9FFF */

/* memory map */
static const write_f writemap[16] = {
	/* 0000-7fff  external cart stuff */
	ext0_write, ext0_write, ext0_write, ext0_write,
	ext1_write, ext1_write, ext1_write, ext1_write,
	/* 8000-9fff  8kB Video Ram */
	vram_write, vram_write,
	/* a000-bfff  external cart stuff */
	ext2_write, ext2_write,
	/* c000-dfff  8kB Work Ram */
	wram_write, wram_write,
	/* e000-fdff  Work Ram Echo (usually unused) */
	wrame_write, /* e000 */
	/* fe00-ffff  CPU stuff */
	fxxx_write /* f000 */
};

/* fxxx range */
static const write_f fxxx_writemap[16] = {
	/* 000-dff  Work Ram Echo */
	wrame_write, wrame_write, wrame_write, wrame_write,
	wrame_write, wrame_write, wrame_write, wrame_write,
	wrame_write, wrame_write, wrame_write, wrame_write,
	wrame_write, wrame_write,
	/* e00-e9f  OAM (160 bytes) */
	/* ea0-eff  NIL */
	oam_write,
	/* f00-fff  CPU */
	cpu_write
};

void wram_write(uint16_t address, uint8_t value) {
	wram[address - 0xC000] = value;
}

void wrame_write(uint16_t address, uint8_t value) {
	wram_write(address - 0x2000, value);
}

void oam_write(uint16_t address, uint8_t value) {
	if(address < 0xfea0) {
		oam[address - 0xfe00] = value;
	}
}

void fxxx_write(uint16_t address, uint8_t value) {
	fxxx_writemap[(address >> 8) & 0xF](address, value);
}

void cpu_write(uint16_t address, uint8_t value) {
	if(address == 0xFFFF) {
		io_write(address, value);
		return;
	}
	if((address >> 7) & 1) {
		hram_write(address, value);
		return;
	}
	io_write(address, value);
}

void hram_write(uint16_t address, uint8_t value) {
	hram[(uint8_t)address - 0x80] = value;
}

void io_write(uint16_t address, uint8_t value) {
}

void write(uint16_t address, uint8_t value) {
	/* Use address blocks to avoid if branching. :D */
	/* With just 16 blocks we can massively reduce the branching here. */
	writemap[address >> 12](address, value);
}

void write16(uint16_t address, uint16_t value) {
	write(address, value);
	write(address + 1, value >> 8);
}
