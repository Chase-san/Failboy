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
  LCDC_BG_ENABLE = 0x01, /* background and window; the DMG blanks both when off, a CGB game only their priority */
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
};

/* object attributes, and on a CGB the background's too (in VRAM bank 1, beside the tile indexes) */
enum {
  ATTR_CGB_PALETTE = 0x07,
  ATTR_BANK = 0x08,    /* CGB: the tile data is in VRAM bank 1 */
  ATTR_PALETTE = 0x10, /* DMG: OBP1 rather than OBP0 */
  ATTR_FLIP_X = 0x20,
  ATTR_FLIP_Y = 0x40,
  ATTR_BEHIND_BG = 0x80, /* an object behind background colors 1-3; on a CGB, background over objects */
};

/* CGB: palettes, 8 each for the background and objects, of 4 colors in 15 bits (5 each of red, green and blue) */
enum {
  PALETTES = 8, /* a line's palette numbers run on through the objects': 8-15 */
  PALETTE_COLORS = 4,
  COLOR_BYTES = 2, /* little-endian */
  PALETTE_RAM_SIZE = PALETTES * PALETTE_COLORS * COLOR_BYTES,
  COLOR_MASK = 0x7FFF,
  WHITE = 0x7FFF,
  BLACK = 0x0000,
  PALETTE_INDEX = 0x3F,     /* BCPS/OCPS: the byte of palette RAM that BCPD/OCPD reach */
  PALETTE_INCREMENT = 0x80, /* moving on to the next after each write */
  PALETTE_SPEC_UNUSED = 0x40,
  VBK_UNUSED = 0xFE,
  OPRI_UNUSED = 0xFE,
};

/* CGB: VRAM DMA, to copy 16-byte blocks into VRAM: all at once, or one at the start of each HBlank */
enum {
  HDMA_BLOCK = 0x10,
  HDMA_BLOCKS = 0x7F,   /* HDMA5: how many, less one */
  HDMA_HBLANK = 0x80,   /* HDMA5 written: a block each HBlank; read: no copy going */
  HDMA_IDLE = 0xFF,     /* HDMA5 read with no copy started, or once it's done */
  HDMA_SOURCE = 0xFFF0, /* the low 4 bits of the source and destination are ignored */
  HDMA_DEST = 0x1FF0,   /* the destination's an offset into VRAM */
  BYTE_BITS = 8,
  BYTE_MASK = 0xFF,
};

/* the Nintendo logo, as the DMG's boot ROM leaves it in VRAM */
enum {
  LOGO_HEADER = 0x0104, /* the cartridge's copy: a nibble for each 4 pixels */
  LOGO_BYTES = 48,
  LOGO_TILES = 0x8010, /* tiles 1-24, each pixel doubled both ways, and then the (R) */
  LOGO_ROW_TILES = 12,
  LOGO_MAP_TOP = 0x9904, /* tiles 1-12 */
  LOGO_MAP_BOTTOM = 0x9924,
  LOGO_MAP_REGISTERED = 0x9910,
  REGISTERED_TILE = 25,
  NIBBLE_BITS = 4,
  NIBBLE_MASK = 0x0F,
};

static const uint8_t registered[TILE_SIZE] = {
    0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
};

/* the palettes a CGB's boot ROM gives an original Game Boy game it doesn't know: background, and both for objects */
static const uint16_t compat_background[PALETTE_COLORS] = {
    0x7FFF, /* white */
    0x1BEF, /* light green */
    0x6180, /* blue */
    0x0000, /* black */
};

static const uint16_t compat_objects[PALETTE_COLORS] = {
    0x7FFF, /* white */
    0x421F, /* pink */
    0x1CF2, /* dark red */
    0x0000, /* black */
};

/* DMG: shades after the palettes, 0 (lightest) to 3 */
static uint8_t framebuffer[LCD_HEIGHT][LCD_WIDTH];
/* CGB: colors from the palettes */
static uint16_t color_frame[LCD_HEIGHT][LCD_WIDTH];
static uint32_t frames = 0;

/* CGB */
static uint8_t vram_bank = 0; /* the one the CPU sees */
static uint8_t bg_palettes[PALETTE_RAM_SIZE];
static uint8_t obj_palettes[PALETTE_RAM_SIZE];
static uint16_t hdma_source = 0;
static uint16_t hdma_dest = 0;  /* offset into VRAM */
static uint8_t hdma_blocks = 0; /* left to copy, a block each HBlank */
static uint8_t hdma_status = HDMA_IDLE;

/* T-cycles into the current line, and ones not yet run (short of an M-cycle, in double speed) */
static uint32_t line_cycles = 0;
static uint32_t spare_cycles = 0;

/* The window keeps its own line counter, which only moves on lines where the window is drawn. */
static uint8_t window_line = 0;
static uint8_t window_triggered = 0; /* LY has matched WY this frame */

/* STAT's enabled interrupt conditions OR'd together; the interrupt fires when this rises. */
static uint8_t stat_signal = 0;

const uint8_t *video_framebuffer(void) { return &framebuffer[0][0]; }

const uint16_t *video_colors(void) { return &color_frame[0][0]; }

uint32_t video_frames(void) { return frames; }

uint8_t vram_read(uint16_t address) { return vram[vram_bank * VRAM_SIZE + address - VRAM_START]; }

void vram_write(uint16_t address, uint8_t value) { vram[vram_bank * VRAM_SIZE + address - VRAM_START] = value; }

/* **************************************** */
/* Rendering */

static force_inline uint8_t shade(uint8_t palette, uint8_t color) {
  return (palette >> (color * SHADE_BITS)) & SHADE_MASK;
}

/* color index (0-3) of one pixel of a tile's row: its low bits, then high bits, with the leftmost pixel's at the top */
static force_inline uint8_t row_pixel(const uint8_t *row, int x) {
  int bit = TILE_SIZE - 1 - x;
  return ((row[1] >> bit) & 1) << 1 | ((row[0] >> bit) & 1);
}

/* a tile row's byte mirrored, for a tile flipped in X */
static force_inline uint8_t reverse_bits(uint8_t bits) {
  bits = (bits & 0xF0) >> 4 | (bits & 0x0F) << 4;
  bits = (bits & 0xCC) >> 2 | (bits & 0x33) << 2;
  return (bits & 0xAA) >> 1 | (bits & 0x55) << 1;
}

/* the VRAM bank a tile's attributes put its data in */
static force_inline const uint8_t *tile_bank(uint8_t attr) {
  if (attr & ATTR_BANK) {
    return vram + VRAM_SIZE;
  }
  return vram;
}

static force_inline uint16_t bg_tile(uint8_t lcdc, uint8_t index) {
  if (lcdc & LCDC_TILE_DATA) {
    return TILE_DATA_0 + index * TILE_BYTES;
  }
  return TILE_DATA_1 + (int8_t)index * TILE_BYTES;
}

/* Pixels start to end - 1 of a line, from a 256x256 tile map starting at its pixel (x, y), a tile at a time: their */
/* color indexes and their tiles' attributes. Those are in VRAM bank 1 beside the map, where only a CGB game writes, */
/* so they're 0 for any other. */
static void render_map(uint8_t lcdc, uint16_t map, uint8_t x, uint8_t y, int start, int end, uint8_t *color,
                       uint8_t *attrs) {
  unsigned int map_row = map - VRAM_START + (y / TILE_SIZE) * MAP_WIDTH;
  int screen_x = start;
  while (screen_x < end) {
    unsigned int cell = map_row + x / TILE_SIZE;
    uint8_t attr = vram[VRAM_SIZE + cell];
    int row = y % TILE_SIZE;
    if (attr & ATTR_FLIP_Y) {
      row = TILE_SIZE - 1 - row;
    }
    const uint8_t *data = tile_bank(attr) + (bg_tile(lcdc, vram[cell]) - VRAM_START) + row * TILE_ROW_BYTES;
    uint8_t bits[TILE_ROW_BYTES] = {data[0], data[1]};
    if (attr & ATTR_FLIP_X) {
      bits[0] = reverse_bits(bits[0]);
      bits[1] = reverse_bits(bits[1]);
    }
    /* the rest of this tile, or of the line */
    int column = x % TILE_SIZE;
    int count = TILE_SIZE - column;
    if (count > end - screen_x) {
      count = end - screen_x;
    }
    for (int i = 0; i < count; ++i) {
      color[screen_x + i] = row_pixel(bits, column + i);
      attrs[screen_x + i] = attr;
    }
    x += count; /* wrapping around the map at 256, the end of a tile */
    screen_x += count;
  }
}

static void render_background(uint8_t lcdc, uint8_t ly, uint8_t *color, uint8_t *attrs) {
  uint16_t map = TILE_MAP_0;
  if (lcdc & LCDC_BG_MAP) {
    map = TILE_MAP_1;
  }
  render_map(lcdc, map, IO_REG(IO_SCX), ly + IO_REG(IO_SCY), 0, LCD_WIDTH, color, attrs);
}

static void render_window(uint8_t lcdc, uint8_t *color, uint8_t *attrs) {
  int left = IO_REG(IO_WX) - WX_OFFSET;
  if (!(lcdc & LCDC_WIN_ENABLE) || !window_triggered || left >= LCD_WIDTH) {
    return;
  }
  uint16_t map = TILE_MAP_0;
  if (lcdc & LCDC_WIN_MAP) {
    map = TILE_MAP_1;
  }
  int start = 0;
  if (left > 0) {
    start = left;
  }
  render_map(lcdc, map, start - left, window_line, start, LCD_WIDTH, color, attrs);
  ++window_line;
}

/* An opaque object pixel over a background pixel of color bg_color with attributes bg_attr: does it show? */
static force_inline int object_shows(uint8_t lcdc, uint8_t attr, uint8_t bg_color, uint8_t bg_attr) {
  if (bg_color == 0) {
    return 1;
  }
  if (model == MODEL_CGB) {
    /* LCDC bit 0 takes the background's priority away, else either one's priority bit puts the background on top */
    return !(lcdc & LCDC_BG_ENABLE) || !((attr | bg_attr) & ATTR_BEHIND_BG);
  }
  return !(attr & ATTR_BEHIND_BG);
}

/* The line's objects over its background (bg_color and bg_attrs), into pixel and palette as render_line has them. */
static void render_sprites(uint8_t lcdc, uint8_t ly, const uint8_t *bg_color, const uint8_t *bg_attrs, uint8_t *pixel,
                           uint8_t *palette) {
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

  /* On the DMG the leftmost sprite wins, then the earliest in OAM: a stable sort by X. A CGB game goes by OAM alone. */
  for (int i = 1; i < count && model != MODEL_CGB; ++i) {
    const uint8_t *obj = found[i];
    int j = i;
    for (; j > 0 && found[j - 1][OBJ_X] > obj[OBJ_X]; --j) {
      found[j] = found[j - 1];
    }
    found[j] = obj;
  }

  if (count == 0) {
    return;
  }

  /* Highest priority first: the first opaque pixel at an x claims it, even when the background then wins it. */
  uint8_t claimed[LCD_WIDTH] = {0};
  for (int i = 0; i < count; ++i) {
    const uint8_t *obj = found[i];
    uint8_t attr = obj[OBJ_ATTR];
    int row = ly - (obj[OBJ_Y] - OBJ_Y_OFFSET);
    if (attr & ATTR_FLIP_Y) {
      row = height - 1 - row;
    }
    uint8_t index = obj[OBJ_TILE];
    if (lcdc & LCDC_OBJ_TALL) {
      index &= OBJ_TALL_TILE_MASK;
    }
    const uint8_t *bank = vram;
    if (model == MODEL_CGB) {
      bank = tile_bank(attr);
    }
    const uint8_t *data =
        bank + (TILE_DATA_0 - VRAM_START) + (index + row / TILE_SIZE) * TILE_BYTES + (row % TILE_SIZE) * TILE_ROW_BYTES;
    uint8_t bits[TILE_ROW_BYTES] = {data[0], data[1]};
    if (attr & ATTR_FLIP_X) {
      bits[0] = reverse_bits(bits[0]);
      bits[1] = reverse_bits(bits[1]);
    }
    int left = obj[OBJ_X] - OBJ_X_OFFSET;
    for (int column = 0; column < TILE_SIZE; ++column) {
      int x = left + column;
      if (x < 0 || x >= LCD_WIDTH || claimed[x]) {
        continue;
      }
      uint8_t color = row_pixel(bits, column);
      if (color == 0) {
        /* transparent, so a lower-priority sprite can show through */
        continue;
      }
      claimed[x] = 1;
      if (!object_shows(lcdc, attr, bg_color[x], bg_attrs[x])) {
        continue;
      }
      if (model == MODEL_CGB) {
        pixel[x] = color;
        palette[x] = PALETTES + (attr & ATTR_CGB_PALETTE);
      } else if (attr & ATTR_PALETTE) {
        pixel[x] = shade(IO_REG(IO_OBP1), color);
        palette[x] = PALETTES + 1;
      } else {
        pixel[x] = shade(IO_REG(IO_OBP0), color);
        palette[x] = PALETTES;
      }
    }
  }
}

/* CGB: color index of a palette, 0-7 in background palette RAM and 8-15 in object palette RAM */
static uint16_t palette_color(uint8_t palette, uint8_t index) {
  const uint8_t *ram = bg_palettes;
  if (palette >= PALETTES) {
    ram = obj_palettes;
    palette -= PALETTES;
  }
  const uint8_t *color = &ram[(palette * PALETTE_COLORS + index) * COLOR_BYTES];
  return (color[0] | color[1] << BYTE_BITS) & COLOR_MASK;
}

static void render_line(uint8_t ly) {
  uint8_t lcdc = IO_REG(IO_LCDC);
  /* the background and window's color indexes and attributes, which sprite priority needs */
  uint8_t color[LCD_WIDTH];
  uint8_t attrs[LCD_WIDTH];
  /* What shows: a color index and its palette, 0-7 the background's, 8-15 the objects'. For a DMG game, the index is */
  /* the shade its palette register gives, and the palette only matters to a CGB running it: 0, 8 for OBP0, 9 OBP1. */
  /* A DMG's shades go straight into the frame. */
  uint8_t indexes[LCD_WIDTH];
  uint8_t palette[LCD_WIDTH];
  uint8_t *pixel = indexes;
  if (model == MODEL_DMG) {
    pixel = framebuffer[ly];
  }

  if (model == MODEL_CGB) {
    render_background(lcdc, ly, color, attrs);
    render_window(lcdc, color, attrs);
    for (int x = 0; x < LCD_WIDTH; ++x) {
      pixel[x] = color[x];
      palette[x] = attrs[x] & ATTR_CGB_PALETTE;
    }
  } else if (lcdc & LCDC_BG_ENABLE) {
    render_background(lcdc, ly, color, attrs);
    render_window(lcdc, color, attrs);
    for (int x = 0; x < LCD_WIDTH; ++x) {
      pixel[x] = shade(IO_REG(IO_BGP), color[x]);
      palette[x] = 0;
    }
  } else {
    memset(color, 0, sizeof(color));
    memset(attrs, 0, sizeof(attrs));
    memset(pixel, 0, LCD_WIDTH);
    memset(palette, 0, sizeof(palette));
  }
  if (lcdc & LCDC_OBJ_ENABLE) {
    render_sprites(lcdc, ly, color, attrs, pixel, palette);
  }

  if (model != MODEL_DMG) {
    for (int x = 0; x < LCD_WIDTH; ++x) {
      color_frame[ly][x] = palette_color(palette[x], pixel[x]);
    }
  }
}

/* **************************************** */
/* CGB VRAM DMA, into the VRAM bank the CPU sees */

static void hdma_copy(void) {
  for (int i = 0; i < HDMA_BLOCK; ++i) {
    vram_write(VRAM_START + ((hdma_dest + i) & (VRAM_SIZE - 1)), mem_read(hdma_source + i));
  }
  hdma_source += HDMA_BLOCK;
  hdma_dest += HDMA_BLOCK;
}

/* the start of an HBlank: a block of an HBlank copy */
static void hdma_hblank(void) {
  if (hdma_blocks) {
    hdma_copy();
    --hdma_blocks;
    hdma_status = hdma_blocks - 1;
    if (hdma_blocks == 0) {
      hdma_status = HDMA_IDLE;
    }
  }
}

static void hdma_start(uint8_t value) {
  uint8_t blocks = (value & HDMA_BLOCKS) + 1;
  if (hdma_blocks && !(value & HDMA_HBLANK)) {
    /* ends an HBlank copy early, and HDMA5 reads back what was written */
    hdma_status = HDMA_HBLANK | value;
    hdma_blocks = 0;
  } else if (value & HDMA_HBLANK) {
    hdma_blocks = blocks;
    hdma_status = hdma_blocks - 1;
    /* with no line being drawn (in HBlank already, or the LCD off), the first block goes right away */
    if ((IO_REG(IO_STAT) & STAT_MODE) == MODE_HBLANK) {
      hdma_hblank();
    }
  } else {
    /* all at once, with the CPU stopped (for no time at all, here) */
    for (; blocks > 0; --blocks) {
      hdma_copy();
    }
    hdma_status = HDMA_IDLE;
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
      hdma_hblank();
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
  for (cycles += spare_cycles; cycles >= M_CYCLE; cycles -= M_CYCLE) {
    video_cycle();
  }
  spare_cycles = cycles;
}

static void fill(uint8_t shade, uint16_t color) {
  memset(framebuffer, shade, sizeof(framebuffer));
  for (int y = 0; y < LCD_HEIGHT; ++y) {
    for (int x = 0; x < LCD_WIDTH; ++x) {
      color_frame[y][x] = color;
    }
  }
}

static void video_blank(void) { fill(0, WHITE); }

/* The clock's stopped: the DMG's screen goes blank, and a CGB's black, its PPU running on without VRAM. */
void video_stop(void) {
  if (model == MODEL_DMG) {
    video_blank();
  } else {
    fill(0, BLACK);
  }
}

static void lcd_switch(uint8_t on) {
  line_cycles = 0;
  IO_REG(IO_LY) = 0;
  if (on) {
    start_line();
  } else {
    video_blank();
    set_mode(MODE_HBLANK);
  }
}

/* each bit of a nibble twice: 4 pixels of the logo as 8 */
static uint8_t double_bits(uint8_t nibble) {
  uint8_t doubled = 0;
  for (int bit = 0; bit < NIBBLE_BITS; ++bit) {
    if (nibble & (1 << bit)) {
      doubled |= 3 << (bit * 2);
    }
  }
  return doubled;
}

static void set_palette(uint8_t *ram, int palette, const uint16_t *colors) {
  for (int i = 0; i < PALETTE_COLORS; ++i) {
    ram[(palette * PALETTE_COLORS + i) * COLOR_BYTES] = colors[i] & BYTE_MASK;
    ram[(palette * PALETTE_COLORS + i) * COLOR_BYTES + 1] = colors[i] >> BYTE_BITS;
  }
}

/* The DMG's boot ROM leaves its logo in VRAM, from the cartridge's copy. */
static void load_logo(void) {
  /* each byte of the logo makes 4 rows of a tile, in the low bit plane: the high nibble twice, then the low one */
  uint8_t *row = &vram[LOGO_TILES - VRAM_START];
  for (int i = 0; i < LOGO_BYTES; ++i) {
    uint8_t logo = mem_read(LOGO_HEADER + i);
    row[0] = row[TILE_ROW_BYTES] = double_bits(logo >> NIBBLE_BITS);
    row[2 * TILE_ROW_BYTES] = row[3 * TILE_ROW_BYTES] = double_bits(logo & NIBBLE_MASK);
    row += 4 * TILE_ROW_BYTES;
  }
  for (int i = 0; i < TILE_SIZE; ++i) {
    row[i * TILE_ROW_BYTES] = registered[i];
  }
  for (int i = 0; i < LOGO_ROW_TILES; ++i) {
    vram[LOGO_MAP_TOP - VRAM_START + i] = 1 + i;
    vram[LOGO_MAP_BOTTOM - VRAM_START + i] = 1 + LOGO_ROW_TILES + i;
  }
  vram[LOGO_MAP_REGISTERED - VRAM_START] = REGISTERED_TILE;
}

/* the screen as the boot ROM leaves it */
void video_bios_init(void) {
  if (model == MODEL_DMG) {
    load_logo();
    return;
  }
  memset(bg_palettes, BYTE_MASK, sizeof(bg_palettes)); /* white */
  memset(obj_palettes, BYTE_MASK, sizeof(obj_palettes));
  if (model == MODEL_CGB_DMG) {
    set_palette(bg_palettes, 0, compat_background);
    set_palette(obj_palettes, 0, compat_objects);
    set_palette(obj_palettes, 1, compat_objects);
  }
  video_blank();
}

/* **************************************** */
/* Registers FF40-FF4B (bar DMA), and a CGB game's VBK, HDMA1-5, BCPS/BCPD, OCPS/OCPD and OPRI */

/* BCPD/OCPD: the byte of palette RAM that BCPS/OCPS (spec) points at, which can then move on */
static void palette_write(uint8_t *ram, uint16_t spec, uint8_t value) {
  uint8_t index = IO_REG(spec) & PALETTE_INDEX;
  ram[index] = value;
  if (IO_REG(spec) & PALETTE_INCREMENT) {
    IO_REG(spec) = (IO_REG(spec) & ~PALETTE_INDEX) | ((index + 1) & PALETTE_INDEX);
  }
}

uint8_t video_read(uint16_t address) {
  switch (address) {
    case IO_STAT:
      return IO_REG(IO_STAT) | STAT_UNUSED;
    case IO_LY:
      if (doctor) {
        return DOCTOR_LY;
      }
      return IO_REG(IO_LY);
    case IO_VBK:
      return VBK_UNUSED | vram_bank;
    case IO_HDMA1:
    case IO_HDMA2:
    case IO_HDMA3:
    case IO_HDMA4:
      return BYTE_MASK; /* write-only */
    case IO_HDMA5:
      return hdma_status;
    case IO_BCPS:
    case IO_OCPS:
      return IO_REG(address) | PALETTE_SPEC_UNUSED;
    case IO_BCPD:
      return bg_palettes[IO_REG(IO_BCPS) & PALETTE_INDEX];
    case IO_OCPD:
      return obj_palettes[IO_REG(IO_OCPS) & PALETTE_INDEX];
    case IO_OPRI:
      return IO_REG(IO_OPRI) | OPRI_UNUSED;
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
    case IO_VBK:
      vram_bank = value & 1;
      break;
    case IO_HDMA1:
      hdma_source = (hdma_source & BYTE_MASK) | value << BYTE_BITS;
      break;
    case IO_HDMA2:
      hdma_source = (hdma_source & ~BYTE_MASK) | (value & HDMA_SOURCE);
      break;
    case IO_HDMA3:
      hdma_dest = ((hdma_dest & BYTE_MASK) | value << BYTE_BITS) & HDMA_DEST;
      break;
    case IO_HDMA4:
      hdma_dest = ((hdma_dest & ~BYTE_MASK) | value) & HDMA_DEST;
      break;
    case IO_HDMA5:
      hdma_start(value);
      break;
    case IO_BCPD:
      palette_write(bg_palettes, IO_BCPS, value);
      break;
    case IO_OCPD:
      palette_write(obj_palettes, IO_OCPS, value);
      break;
    default:
      IO_REG(address) = value;
      break;
  }
}
