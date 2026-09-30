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

enum {
  LCDC_ENABLE = 0x80,
  LCD_HEIGHT = 144,  /* visible lines; VBlank starts after them */
  LCD_LINES = 154,   /* lines per frame, including VBlank */
  LINE_CYCLES = 456, /* T-cycles per line */
};

/* T-cycles into the current line */
static uint32_t line_cycles = 0;

uint8_t vram_read(uint16_t address) { return vram[address - VRAM_START]; }

void vram_write(uint16_t address, uint8_t value) { vram[address - VRAM_START] = value; }

/* Only LY timing so far. */
void video_tick(uint32_t cycles) {
  if (!(IO_REG(IO_LCDC) & LCDC_ENABLE)) {
    IO_REG(IO_LY) = 0;
    line_cycles = 0;
    return;
  }
  line_cycles += cycles;
  while (line_cycles >= LINE_CYCLES) {
    line_cycles -= LINE_CYCLES;
    if (++IO_REG(IO_LY) == LCD_LINES) {
      IO_REG(IO_LY) = 0;
    }
    if (IO_REG(IO_LY) == LCD_HEIGHT) {
      io_request(INT_VBLANK);
    }
  }
}
