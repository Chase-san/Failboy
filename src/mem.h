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

#ifndef _MEM_H_
#define _MEM_H_

#include <stdint.h>

/* cart.h */
uint8_t ext0_read(uint16_t); /* 0000-3FFF */
uint8_t ext1_read(uint16_t); /* 4000-7FFF */
uint8_t ext2_read(uint16_t); /* A000-BFFF */

void ext0_write(uint16_t, uint8_t); /* 0000-3FFF */
void ext1_write(uint16_t, uint8_t); /* 4000-7FFF */
void ext2_write(uint16_t, uint8_t); /* A000-BFFF */

/* io.c */
uint8_t io_read(uint16_t);        /* FF00-FF7F */
void io_write(uint16_t, uint8_t); /* FF80-FFFE */

/* video.c */
uint8_t vram_read(uint16_t);        /* 8000-9FFF */
void vram_write(uint16_t, uint8_t); /* 8000-9FFF */

/* READ */
uint8_t wram_read(uint16_t);  /* C000-DFFF */
uint8_t wrame_read(uint16_t); /* E000-FDFF */
uint8_t oam_read(uint16_t);   /* FE00-FE9F */
uint8_t fxxx_read(uint16_t);  /* F000-FFFF */
uint8_t cpu_read(uint16_t);   /* FF00-FFFF */
uint8_t hram_read(uint16_t);  /* FF80-FFFE */

/* WRITE */
void wram_write(uint16_t, uint8_t);  /* C000-DFFF */
void wrame_write(uint16_t, uint8_t); /* E000-FDFF */
void oam_write(uint16_t, uint8_t);   /* FE00-FE9F */
void fxxx_write(uint16_t, uint8_t);  /* F000-FFFF */
void cpu_write(uint16_t, uint8_t);   /* FF00-FFFF */
void hram_write(uint16_t, uint8_t);  /* FF00-FF7F */
void vram_write(uint16_t, uint8_t);  /* 8000-9FFF */

#endif /* _MEM_H_ */