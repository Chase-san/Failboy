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

#include <stdlib.h>

#include "failboy.h"

/* ************************************************************** */
/* cart.c */
uint8_t ext0_read(uint16_t); /* 0000-3FFF */
uint8_t ext1_read(uint16_t); /* 4000-7FFF */
uint8_t ext2_read(uint16_t); /* A000-BFFF */

void ext0_write(uint16_t, uint8_t); /* 0000-3FFF */
void ext1_write(uint16_t, uint8_t); /* 4000-7FFF */
void ext2_write(uint16_t, uint8_t); /* A000-BFFF */

/* io.c */
uint8_t io_read(uint16_t);        /* FF00-FF7F, FFFF */
void io_write(uint16_t, uint8_t); /* FF00-FF7F, FFFF */

/* video.c */
uint8_t vram_read(uint16_t);        /* 8000-9FFF */
void vram_write(uint16_t, uint8_t); /* 8000-9FFF */

/* I use function pointers here to make things easier, faster and simpler. Built in bound checking is nice as well. */

/* ************************************************************** */

static uint8_t *wram;
static uint8_t *hram;
uint8_t *oam;
uint8_t *vram;

/* where D000-DFFF is in wram: bank 1, unless a CGB picks another */
static unsigned int wram_bank_offset = WRAM_BANK_SIZE;

int mem_alloc(void) {
  /* 8 kB of work RAM (32 kB on a CGB), 8 kB of video RAM (16 kB) */
  wram = calloc(WRAM_BANKS, WRAM_BANK_SIZE);
  hram = calloc(1, HRAM_SIZE);
  oam = calloc(1, OAM_SIZE);
  vram = calloc(VRAM_BANKS, VRAM_SIZE);
  if (wram == NULL || hram == NULL || oam == NULL || vram == NULL) {
    mem_free();
    return 0;
  }
  return 1;
}

void mem_free(void) {
  free(vram);
  free(oam);
  free(hram);
  free(wram);
  vram = oam = hram = wram = NULL;
}

void mem_wram_bank(uint8_t bank) {
  if (bank == 0) {
    bank = 1;
  }
  wram_bank_offset = bank * WRAM_BANK_SIZE;
}

/* ************************************************************** */
/* READ */
uint8_t wram_read(uint16_t);  /* C000-DFFF */
uint8_t wrame_read(uint16_t); /* E000-FDFF */
uint8_t oam_read(uint16_t);   /* FE00-FE9F */
uint8_t fxxx_read(uint16_t);  /* F000-FFFF */
uint8_t cpu_read(uint16_t);   /* FF00-FFFF */
uint8_t hram_read(uint16_t);  /* FF80-FFFE */

/* memory map */
static const read_f readmap[16] = {
    /* 0000-3fff  external cart 0 */
    ext0_read,
    ext0_read,
    ext0_read,
    ext0_read,
    /* 4000-7fff  external cart 0 */
    ext1_read,
    ext1_read,
    ext1_read,
    ext1_read,
    /* 8000-9fff  8kB Video Ram */
    vram_read,
    vram_read,
    /* a000-bfff  external cart stuff */
    ext2_read,
    ext2_read,
    /* c000-dfff  8kB Work Ram */
    wram_read,
    wram_read,
    /* e000-fdff  Work Ram Echo (usually unused) */
    wrame_read, /* e000 */
    /* fe00-ffff  CPU stuff */
    fxxx_read, /* f000 */
};

/* fxxx range */
static const read_f fxxx_readmap[16] = {
    /* 000h-dffh  Work Ram Echo */
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    wrame_read,
    /* e00-e9f  OAM (160 bytes) */
    /* ea0-eff  NIL */
    oam_read,
    /* f00-fff  CPU */
    cpu_read,
};

uint8_t wram_read(uint16_t address) {
  if (address < WRAM_START + WRAM_BANK_SIZE) {
    return wram[address - WRAM_START];
  }
  return wram[wram_bank_offset + address - (WRAM_START + WRAM_BANK_SIZE)];
}

uint8_t wrame_read(uint16_t address) { return wram_read(address - (ECHO_START - WRAM_START)); }

uint8_t oam_read(uint16_t address) {
  if (address < OAM_START + OAM_SIZE) {
    return oam[address - OAM_START];
  }
  return 0;
}

uint8_t fxxx_read(uint16_t address) { return fxxx_readmap[(address >> 8) & 0xF](address); }

uint8_t cpu_read(uint16_t address) {
  if (address == IO_IE) {
    return io_read(address);
  }
  if (address >= HRAM_START) {
    return hram_read(address);
  }
  return io_read(address);
}

uint8_t hram_read(uint16_t address) { return hram[address - HRAM_START]; }

uint8_t mem_read(uint16_t address) {
  /* Use address blocks to avoid if branching. :D */
  /* With just 16 blocks we can massively reduce the branching here. */
  return readmap[address >> 12](address);
}

uint16_t mem_read16(uint16_t address) { return (mem_read(address)) | (mem_read(address + 1) << 8); }

/* ************************************************************** */
/* WRITE */
void wram_write(uint16_t, uint8_t);  /* C000-DFFF */
void wrame_write(uint16_t, uint8_t); /* E000-FDFF */
void oam_write(uint16_t, uint8_t);   /* FE00-FE9F */
void fxxx_write(uint16_t, uint8_t);  /* F000-FFFF */
void cpu_write(uint16_t, uint8_t);   /* FF00-FFFF */
void hram_write(uint16_t, uint8_t);  /* FF80-FFFE */
void vram_write(uint16_t, uint8_t);  /* 8000-9FFF */

/* memory map */
static const write_f writemap[16] = {
    /* 0000-7fff  external cart stuff */
    ext0_write,
    ext0_write,
    ext0_write,
    ext0_write,
    ext1_write,
    ext1_write,
    ext1_write,
    ext1_write,
    /* 8000-9fff  8kB Video Ram */
    vram_write,
    vram_write,
    /* a000-bfff  external cart stuff */
    ext2_write,
    ext2_write,
    /* c000-dfff  8kB Work Ram */
    wram_write,
    wram_write,
    /* e000-fdff  Work Ram Echo (usually unused) */
    wrame_write, /* e000 */
    /* fe00-ffff  CPU stuff */
    fxxx_write, /* f000 */
};

/* fxxx range */
static const write_f fxxx_writemap[16] = {
    /* 000-dff  Work Ram Echo */
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    wrame_write,
    /* e00-e9f  OAM (160 bytes) */
    /* ea0-eff  NIL */
    oam_write,
    /* f00-fff  CPU */
    cpu_write,
};

void wram_write(uint16_t address, uint8_t value) {
  if (address < WRAM_START + WRAM_BANK_SIZE) {
    wram[address - WRAM_START] = value;
  } else {
    wram[wram_bank_offset + address - (WRAM_START + WRAM_BANK_SIZE)] = value;
  }
}

void wrame_write(uint16_t address, uint8_t value) { wram_write(address - (ECHO_START - WRAM_START), value); }

void oam_write(uint16_t address, uint8_t value) {
  if (address < OAM_START + OAM_SIZE) {
    oam[address - OAM_START] = value;
  }
}

void fxxx_write(uint16_t address, uint8_t value) { fxxx_writemap[(address >> 8) & 0xF](address, value); }

void cpu_write(uint16_t address, uint8_t value) {
  if (address == IO_IE) {
    io_write(address, value);
    return;
  }
  if (address >= HRAM_START) {
    hram_write(address, value);
    return;
  }
  io_write(address, value);
}

void hram_write(uint16_t address, uint8_t value) { hram[address - HRAM_START] = value; }

void mem_write(uint16_t address, uint8_t value) {
  /* Use address blocks to avoid if branching. :D */
  /* With just 16 blocks we can massively reduce the branching here. */
  writemap[address >> 12](address, value);
}

void mem_write16(uint16_t address, uint16_t value) {
  mem_write(address, value);
  mem_write(address + 1, value >> 8);
}
