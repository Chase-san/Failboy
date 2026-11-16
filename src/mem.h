/**
 * Copyright 2014-2026 Robert Maupin <chasesan@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
 * to permit persons to whom the Software is furnished to do so.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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