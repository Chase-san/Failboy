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

#include <string.h>

#include "failboy.h"

enum {
  LCDC_BG_ENABLE = 0x01, /* background and window; the DMG blanks both when off */
  LCDC_OBJ_ENABLE = 0x02,
  LCDC_OBJ_TALL = 0x04,  /* 8x16 sprites */
  LCDC_BG_MAP = 0x08,    /* background tile map at 9C00 rather than 9800 */
  LCDC_TILE_DATA = 0x10, /* tiles at 8000 with unsigned indexes rather than 8800 with signed ones */
  LCDC_WIN_ENABLE = 0x20,
  LCDC_WIN_MAP = 0x40, /* window tile map at 9C00 rather than 9800 */
  LCDC_ENABLE = 0x80,
};

enum {
  STAT_MODE = 0x03,
  STAT_LYC_EQUAL = 0x04,
  STAT_INT_HBLANK = 0x08,
  STAT_INT_VBLANK = 0x10,
  STAT_INT_OAM = 0x20,
  STAT_INT_LYC = 0x40,
  STAT_WRITABLE = 0x78,
  STAT_UNUSED = 0x80,
};

enum {
  MODE_HBLANK = 0,
  MODE_VBLANK = 1,
  MODE_OAM_SCAN = 2,
  MODE_DRAWING = 3,
};

enum {
  OAM_SCAN_CYCLES = 80,
  DRAWING_CYCLES = 172, /* the shortest mode 3; the real one stretches with scrolling, the window and sprites */
  DOCTOR_LY = 0x90,     /* Gameboy Doctor's logs were made with LY stuck here */
};

enum {
  TILE_MAP_0 = 0x9800,
  TILE_MAP_1 = 0x9C00,
  TILE_DATA_0 = 0x8000, /* unsigned tile indexes */
  TILE_DATA_1 = 0x9000, /* signed tile indexes, so 8800-97FF */
  TILE_SIZE = 8,        /* pixels square */
  TILE_BYTES = 16,
  TILE_ROW_BYTES = 2, /* low bits, then high bits, of 8 pixels */
  MAP_WIDTH = 32,     /* tiles */
  WX_OFFSET = 7,
  SHADE_BITS = 2, /* palettes hold one 2-bit shade per color index */
  SHADE_MASK = 0x03,
};

enum {
  OBJ_COUNT = 40,
  OBJ_BYTES = 4,
  OBJ_Y = 0,
  OBJ_X = 1,
  OBJ_TILE = 2,
  OBJ_ATTR = 3,
  OBJ_PER_LINE = 10,
  OBJ_Y_OFFSET = 16,
  OBJ_X_OFFSET = 8,
  OBJ_TALL_TILE_MASK = 0xFE, /* 8x16 sprites ignore bit 0 of the tile index */
  ATTR_PALETTE = 0x10,
  ATTR_FLIP_X = 0x20,
  ATTR_FLIP_Y = 0x40,
  ATTR_BEHIND_BG = 0x80, /* behind background colors 1-3 */
};

/* shades after the palettes, 0 (lightest) to 3 */
static uint8_t framebuffer[LCD_HEIGHT][LCD_WIDTH];
static uint32_t frames = 0;

/* T-cycles into the current line */
static uint32_t line_cycles = 0;

/* The window keeps its own line counter, which only moves on lines where the window is drawn. */
static uint8_t window_line = 0;
static uint8_t window_triggered = 0; /* LY has matched WY this frame */

/* STAT's enabled interrupt conditions OR'd together; the interrupt fires when this rises. */
static uint8_t stat_signal = 0;

const uint8_t *video_framebuffer(void) { return &framebuffer[0][0]; }

uint32_t video_frames(void) { return frames; }

uint8_t vram_read(uint16_t address) { return vram[address - VRAM_START]; }

void vram_write(uint16_t address, uint8_t value) { vram[address - VRAM_START] = value; }

/* **************************************** */
/* Rendering */

static force_inline uint8_t shade(uint8_t palette, uint8_t color) {
  return (palette >> (color * SHADE_BITS)) & SHADE_MASK;
}

/* color index (0-3) of one pixel of the tile whose data is at tile */
static force_inline uint8_t tile_pixel(uint16_t tile, uint8_t x, uint8_t y) {
  uint8_t lo = vram[tile - VRAM_START + y * TILE_ROW_BYTES];
  uint8_t hi = vram[tile - VRAM_START + y * TILE_ROW_BYTES + 1];
  uint8_t bit = TILE_SIZE - 1 - x;
  return ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
}

static force_inline uint16_t bg_tile(uint8_t lcdc, uint8_t index) {
  if (lcdc & LCDC_TILE_DATA) {
    return TILE_DATA_0 + index * TILE_BYTES;
  }
  return TILE_DATA_1 + (int8_t)index * TILE_BYTES;
}

/* color index of pixel (x, y) of a 256x256 tile map */
static force_inline uint8_t map_pixel(uint8_t lcdc, uint16_t map, uint8_t x, uint8_t y) {
  uint8_t index = vram[map - VRAM_START + (y / TILE_SIZE) * MAP_WIDTH + x / TILE_SIZE];
  return tile_pixel(bg_tile(lcdc, index), x % TILE_SIZE, y % TILE_SIZE);
}

static void render_background(uint8_t lcdc, uint8_t ly, uint8_t *color) {
  uint16_t map = TILE_MAP_0;
  if (lcdc & LCDC_BG_MAP) {
    map = TILE_MAP_1;
  }
  uint8_t y = ly + IO_REG(IO_SCY);
  for (int x = 0; x < LCD_WIDTH; ++x) {
    color[x] = map_pixel(lcdc, map, x + IO_REG(IO_SCX), y);
  }
}

static void render_window(uint8_t lcdc, uint8_t *color) {
  int left = IO_REG(IO_WX) - WX_OFFSET;
  if (!(lcdc & LCDC_WIN_ENABLE) || !window_triggered || left >= LCD_WIDTH) {
    return;
  }
  uint16_t map = TILE_MAP_0;
  if (lcdc & LCDC_WIN_MAP) {
    map = TILE_MAP_1;
  }
  int x = 0;
  if (left > 0) {
    x = left;
  }
  for (; x < LCD_WIDTH; ++x) {
    color[x] = map_pixel(lcdc, map, x - left, window_line);
  }
  ++window_line;
}

static void render_sprites(uint8_t lcdc, uint8_t ly, const uint8_t *bg_color, uint8_t *line) {
  int height = TILE_SIZE;
  if (lcdc & LCDC_OBJ_TALL) {
    height = TILE_SIZE * 2;
  }

  /* the first 10 sprites in OAM that cover this line */
  const uint8_t *found[OBJ_PER_LINE];
  int count = 0;
  for (int i = 0; i < OBJ_COUNT && count < OBJ_PER_LINE; ++i) {
    const uint8_t *obj = &oam[i * OBJ_BYTES];
    int top = obj[OBJ_Y] - OBJ_Y_OFFSET;
    if (ly >= top && ly < top + height) {
      found[count++] = obj;
    }
  }

  /* On the DMG the leftmost sprite wins, then the earliest in OAM: a stable sort by X. */
  for (int i = 1; i < count; ++i) {
    const uint8_t *obj = found[i];
    int j = i;
    for (; j > 0 && found[j - 1][OBJ_X] > obj[OBJ_X]; --j) {
      found[j] = found[j - 1];
    }
    found[j] = obj;
  }

  for (int x = 0; x < LCD_WIDTH; ++x) {
    for (int i = 0; i < count; ++i) {
      const uint8_t *obj = found[i];
      int column = x - (obj[OBJ_X] - OBJ_X_OFFSET);
      if (column < 0 || column >= TILE_SIZE) {
        continue;
      }
      uint8_t attr = obj[OBJ_ATTR];
      int row = ly - (obj[OBJ_Y] - OBJ_Y_OFFSET);
      if (attr & ATTR_FLIP_X) {
        column = TILE_SIZE - 1 - column;
      }
      if (attr & ATTR_FLIP_Y) {
        row = height - 1 - row;
      }
      uint8_t index = obj[OBJ_TILE];
      if (lcdc & LCDC_OBJ_TALL) {
        index &= OBJ_TALL_TILE_MASK;
      }
      uint16_t tile = TILE_DATA_0 + (index + row / TILE_SIZE) * TILE_BYTES;
      uint8_t color = tile_pixel(tile, column, row % TILE_SIZE);
      if (color == 0) {
        /* transparent, so a lower-priority sprite can show through */
        continue;
      }
      /* The highest-priority opaque sprite claims the pixel, even when the background then wins it. */
      if (!(attr & ATTR_BEHIND_BG) || bg_color[x] == 0) {
        uint8_t palette = IO_REG(IO_OBP0);
        if (attr & ATTR_PALETTE) {
          palette = IO_REG(IO_OBP1);
        }
        line[x] = shade(palette, color);
      }
      break;
    }
  }
}

static void render_line(uint8_t ly) {
  uint8_t lcdc = IO_REG(IO_LCDC);
  uint8_t *line = framebuffer[ly];
  /* background/window color indexes, which sprite priority needs */
  uint8_t color[LCD_WIDTH] = {0};

  if (lcdc & LCDC_BG_ENABLE) {
    render_background(lcdc, ly, color);
    render_window(lcdc, color);
    for (int x = 0; x < LCD_WIDTH; ++x) {
      line[x] = shade(IO_REG(IO_BGP), color[x]);
    }
  } else {
    memset(line, 0, LCD_WIDTH);
  }
  if (lcdc & LCDC_OBJ_ENABLE) {
    render_sprites(lcdc, ly, color, line);
  }
}

/* **************************************** */
/* Timing */

static void update_stat(void) {
  uint8_t stat = IO_REG(IO_STAT);
  uint8_t mode = stat & STAT_MODE;
  uint8_t signal = 0;
  if (IO_REG(IO_LCDC) & LCDC_ENABLE) {
    signal = ((stat & STAT_INT_LYC) && (stat & STAT_LYC_EQUAL)) || ((stat & STAT_INT_HBLANK) && mode == MODE_HBLANK) ||
             ((stat & STAT_INT_VBLANK) && mode == MODE_VBLANK) || ((stat & STAT_INT_OAM) && mode == MODE_OAM_SCAN);
  }
  if (signal && !stat_signal) {
    io_request(INT_STAT);
  }
  stat_signal = signal;
}

static void set_mode(uint8_t mode) {
  IO_REG(IO_STAT) = (IO_REG(IO_STAT) & ~STAT_MODE) | mode;
  update_stat();
}

static void compare_lyc(void) {
  if (IO_REG(IO_LY) == IO_REG(IO_LYC)) {
    IO_REG(IO_STAT) |= STAT_LYC_EQUAL;
  } else {
    IO_REG(IO_STAT) &= ~STAT_LYC_EQUAL;
  }
  update_stat();
}

/* LY has just moved on to a new line */
static void start_line(void) {
  uint8_t ly = IO_REG(IO_LY);
  if (ly == 0) {
    window_line = 0;
    window_triggered = 0;
  }
  if (ly < LCD_HEIGHT) {
    if (ly == IO_REG(IO_WY)) {
      window_triggered = 1;
    }
    set_mode(MODE_OAM_SCAN);
  } else if (ly == LCD_HEIGHT) {
    set_mode(MODE_VBLANK);
    io_request(INT_VBLANK);
    ++frames;
  }
  compare_lyc();
}

/* one M-cycle of the PPU */
static void video_cycle(void) {
  line_cycles += M_CYCLE;
  if (IO_REG(IO_LY) < LCD_HEIGHT) {
    if (line_cycles == OAM_SCAN_CYCLES) {
      set_mode(MODE_DRAWING);
    } else if (line_cycles == OAM_SCAN_CYCLES + DRAWING_CYCLES) {
      render_line(IO_REG(IO_LY));
      set_mode(MODE_HBLANK);
    }
  }
  if (line_cycles == LINE_CYCLES) {
    line_cycles = 0;
    if (++IO_REG(IO_LY) == LCD_LINES) {
      IO_REG(IO_LY) = 0;
    }
    start_line();
  }
}

void video_tick(uint32_t cycles) {
  if (!(IO_REG(IO_LCDC) & LCDC_ENABLE)) {
    return;
  }
  for (; cycles >= M_CYCLE; cycles -= M_CYCLE) {
    video_cycle();
  }
}

static void lcd_switch(uint8_t on) {
  line_cycles = 0;
  IO_REG(IO_LY) = 0;
  if (on) {
    start_line();
  } else {
    /* a switched-off LCD is blank */
    memset(framebuffer, 0, sizeof(framebuffer));
    set_mode(MODE_HBLANK);
  }
}

/* **************************************** */
/* Registers FF40-FF4B (bar DMA) */

uint8_t video_read(uint16_t address) {
  switch (address) {
    case IO_STAT:
      return IO_REG(IO_STAT) | STAT_UNUSED;
    case IO_LY:
      if (doctor) {
        return DOCTOR_LY;
      }
      return IO_REG(IO_LY);
    default:
      return IO_REG(address);
  }
}

void video_write(uint16_t address, uint8_t value) {
  uint8_t switched;
  switch (address) {
    case IO_LCDC:
      switched = (value ^ IO_REG(IO_LCDC)) & LCDC_ENABLE;
      IO_REG(IO_LCDC) = value;
      if (switched) {
        lcd_switch(value & LCDC_ENABLE);
      }
      break;
    case IO_STAT:
      IO_REG(IO_STAT) = (IO_REG(IO_STAT) & ~STAT_WRITABLE) | (value & STAT_WRITABLE);
      update_stat();
      break;
    case IO_LY:
      /* read-only */
      break;
    case IO_LYC:
      IO_REG(IO_LYC) = value;
      if (IO_REG(IO_LCDC) & LCDC_ENABLE) {
        compare_lyc();
      }
      break;
    default:
      IO_REG(address) = value;
      break;
  }
}
