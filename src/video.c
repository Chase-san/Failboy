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

/* The PPU, a dot (T-cycle) at a time. A line is the OAM scan, which lists up to 10 objects by X; then the drawing, */
/* where a fetcher reads the background's or window's tiles a row at a time into one FIFO, objects' rows are fetched */
/* into another as the pixels reach them (which holds the pixels up), and a pixel a dot comes out of both through */
/* LCDC and the palettes as they are at that dot; then HBlank. So mode 3 is as long as the scroll, the window and */
/* the objects make it, and a register written partway along a line changes the rest of it. */
/* */
/* What a DMG does is taken from tests that give their own figures: Mooneye's acceptance/ppu for when LY, the modes, */
/* the interrupts and the locks on VRAM and OAM come, and mealybug-tearoom-tests' pictures (from a DMG-CPU B "blob") */
/* for the pixels. The shootout battery's mealybug and Mooneye ppu tests, dmg-acid2 and strikethrough all pass. */
/* */
/* The CGB is one of two revisions, a CPU CGB C or a CPU CGB E (cgb_e), which share all of the PPU but the few */
/* places below. Nobody here can measure either, so each is held to other people's tests of it, and where those */
/* disagree about one revision, the reading that passes more of them is the one kept: */
/* - A CGB C to Gambatte's hwtests (taken to be from a CGB-CPU-04), AGE's tests named for B and C, and mealybug's */
/*   CGB-C pictures: 2420 of Gambatte's 3023 CGB results, 22 of AGE's 37, 7 of mealybug's 27 (2026-11-16). */
/* - A CGB E to DocBoy's test suite (whose CGB matches mealybug's CGB-D pictures wherever those differ from the */
/*   CGB-C ones, and whose author writes of a real CGB-E), AGE's tests named for E, and mealybug's CGB-D pictures, D */
/*   and E being taken as one: 1742 of the 2130 DocBoy tests that aren't of sound and 314 of 607 with a DMG game */
/*   in, 22 of AGE's 37, 6 of mealybug's 20. That DocBoy's results are from that hardware is its author's word, */
/*   in one comment; its README doesn't say. */
/* A test of one revision is applied to the other too, unless the other's own tests say otherwise. What every CGB */
/* has of its own: palette RAM locked while the pixels go (DOT_PALETTES), the mode 0 that STAT shows coming a dot */
/* later at double speed, LCDC's tile data glitch, OAM closed to writes from a line's first dot, and VRAM open to */
/* reads until the drawing. Where the two revisions part: */
/* - A CGB C shows a DMG's edges between lines to a CPU at the normal speed (dmg_edges()): the LY=LYC flag dropping */
/*   for a dot, mode 0 for line 0's first dot, LY reading 0 from line 153's dot 3. At double speed it doesn't, and */
/*   a CGB E never does. AGE's ly and stat-mode tests come in a B/C and an E version, and each revision passes its */
/*   own. */
/* - Line 153 stops comparing LY for a while before it compares 0, but not on a CGB E. */
/* - A write to BGP, OBP0 or OBP1 lands a dot before the end of its M-cycle on a CGB C and two on a CGB E. */
/* */
/* Where it's known to differ from the hardware, or is a guess: */
/* - The OAM bug isn't emulated: on a DMG, a 16-bit INC or DEC (and some other instructions) of a register pointing */
/*   into FE00-FEFF during the OAM scan corrupts OAM. Blargg's oam_bug 2, 4, 5 and 8 fail for it; it needs the CPU's */
/*   instructions to tell the PPU. */
/* - Either CGB, by what still fails of its evidence: mode 3 writes to SCX, SCY, LCDC's map and tile data bits and */
/*   the object bits (mealybug's pictures; of the tile data glitch only one case is done, a tile index under 128 */
/*   read as its own row, and about 110 of DocBoy's pictures are of it); the window and LYC being switched on and */
/*   off (Gambatte's window, lycEnable, m0enable and m2enable); the LCD coming on at odd moments (lcd_offset, */
/*   display_startstate, enable_display); and what a speed switch does to the PPU and the timer (Gambatte's */
/*   speedchange, AGE's spsw, and 194 DocBoy tests of LY after switching there and back). AGE's oam-read and */
/*   lcd-align-ly fail in both their versions. */
/* - Tests that lost to a majority: on a CGB C, four of Gambatte's results that passed before the edges above and */
/*   SCY and SCX landing a dot early (16 came for them): lcd_offset's offset2_lyc98int_ly_count 1 and 3 and its */
/*   offset1_lyc8fint_m1stat_ds_1, and enable_display's ly0_late_scx7_m3stat_scx0_2. Nothing on a CGB E. */
/* - Three of the shootout battery's CGB tests (cgb-acid-hell, daid's ppu_scanline_bgp on a GBC and its */
/*   speed_switch_timing_ly) once passed only with numbers fitted to them: the PPU running 2 dots ahead of the CPU, */
/*   its pixels starting 2 dots sooner, palette writes landing 2 dots early. What they wanted was a halted CGB */
/*   waking an M-cycle later (halt_cycle, cpu.c), as all three wait in HALT; with that they pass as a CGB E, and */
/*   all but ppu_scanline_bgp as a CGB C, whose palette writes land a dot later. DocBoy's author says */
/*   cgb-acid-hell fails on a real CGB-E, which this doesn't bear out. */
/* - That a CGB C's edges go unseen at double speed (dmg_edges()) is what Gambatte's results ask for, not a known */
/*   mechanism: it may be when a CPU at that speed reads, and not the PPU doing anything else. */
/* - OPRI isn't used: a CGB game's objects always go by their place in OAM, any other's by X. */
/* - A DMG's write to STAT doesn't switch all its interrupt sources on for a moment, as the real one's does. */
/* - A window starting a pixel late (window_check) puts the LCD a pixel out of step with the PPU on some DMGs. It */
/*   doesn't here, which is what mealybug's "blob" pictures show, and not its m3_lcdc_win_en_change_multiple_wx */
/*   picture from a DMG-CPU B. */
/* - The window at WX 166 only moves its line counter (DMG); the real one's is said to start during HBlank. WY is */
/*   compared twice a line (as the pixels start, and near the line's end), where the hardware may do it more often. */
/* - DMG, by Gambatte's results for a DMG-CPU-08 and AGE's for a DMG-CPU C (1382 of 1651, and 10 of 14): some of the */
/*   window's and mode 1's cases, AGE's m3-bg-scx picture and a 2-pixel difference in its m3-bg-bgp, and its */
/*   stat-mode-sprites and oam-write. */
/* - The PPU's reads of OAM during OAM DMA (oam_peek) are fitted to strikethrough alone: only the picture that test */
/*   wants was found, by trying models against its tables. DMA's last byte isn't covered, and at double speed which */
/*   half of the M-cycle it is, is a guess. Gambatte's oamdma results mostly fail, on either machine. */
/* - VRAM DMA stops the CPU between two instructions and copies a byte every 2 dots (bus_dma, cpu.c), an HBlank */
/*   copy's block asked for a dot after the HBlank interrupt; that much DocBoy's timing tests and Gambatte's dma */
/*   results agree on. What isn't done: a copy that runs into the drawing loses every byte, where the hardware lets */
/*   one in four through between the fetcher's reads (83 DocBoy tests); OAM DMA running at the same time isn't */
/*   allowed for (31); the first byte of a copy that can't read its source is the next opcode, which isn't right */
/*   when the code is running from RAM; and a block that comes due while the CPU is halted waits for it to wake */
/*   and goes after the instruction that follows, which the tests are split on. */
/* - STOP doesn't leave a CGB's PPU running: the screen goes black, or is left as it was if STOP came in mode 3. */
/* - The frame after the LCD comes on is shown, where a real LCD skips it; and with the LCD off the screen is blank */
/*   at once. */
/* - Where the boot ROM leaves the PPU (BOOT_LINE_DMG and the rest) is worked out from tests that read STAT and LY */
/*   a counted number of M-cycles after start-up: DocBoy's boot tests, with which Gambatte's display_startstate */
/*   results agree, for a DMG and for both CGBs. The other state it leaves isn't: on a CGB most of DocBoy's boot */
/*   tests still fail (the timer by the game's title, the colors given to a DMG game, HRAM, sound). */
/* - A line with more to fetch than there's time for is cut off at its end, rather than running into the next. */
/* - Outside this file: when a halted CPU looks for interrupts (halt_cycle, cpu.c) and how long a speed switch */
/*   stops it (STOP, cpu_instr.c) are both what other people's tests agree on most, tried a step at a time, and not */
/*   known mechanisms. A speed switch can leave the CPU's M-cycles on other dots of a line than the one this file */
/*   was written around (M_CYCLE_DOT); LY read on the dot it changes is done for that, and nothing else: of */
/*   Gambatte's 62 lcd_offset results for a CGB C, which test just that, 25 fail. */

#include <string.h>

#include "core.h"

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

/* Where things happen in a line, in dots (T-cycles) from the one in which LY moves on. What's done in a dot, the CPU */
/* sees from the next; its M-cycles end a dot into the line, and every 4 from there. */
enum {
  DOT_LINE = 0,       /* LY moves on, OAM is taken from the CPU's reads, and the OAM interrupt is asked for */
  DOT_SCAN = 1,       /* mode 2 */
  DOT_SCAN_FIRST = 3, /* the scan finishes with an object every other dot from here */
  DOT_SCAN_EACH = 2,
  DOT_SCAN_RELEASE = 77, /* the DMG's VRAM goes to the PPU, with 2 objects still to scan */
  DOT_DRAWING = 81,      /* mode 3, as the scan ends */
  DOT_PIXELS = 86,       /* the fetcher and the pixels start: mode 3 is 172 dots at its shortest, HBlank from 253 */
  DOT_WINDOW_Y = 454,    /* a second look at WY, the first being as the pixels start */
  DOT_VBLANK_COMPARE = 1,
  DOT_VBLANK = 2,       /* on line 144: mode 1, and the interrupt */
  DOT_LAST_ZERO = 3,    /* on line 153: LY reads 0 from here where a DMG's edges show, though compared as 153 */
  DOT_LAST_NONE = 5,    /* then as nothing, bar a CGB E's (and LY reads 0 from here where they don't show) */
  DOT_LAST_COMPARE = 9, /* and then as 0 */
  NO_DOT = 0xFFFF,
  LAST_LINE = LCD_LINES - 1,
  FIRST_LINE_DOT = 5,        /* where the line is as the M-cycle of the write that switched the LCD on ends */
  FIRST_LINE_LATE = 2,       /* dots late that line's drawing is */
  BOOT_LINE_DMG = LAST_LINE, /* where the boot ROM leaves the PPU: a DMG's, 14 M-cycles before line 0, */
  BOOT_DOT_DMG = 401,
  BOOT_LINE_CGB = LCD_HEIGHT, /* and a CGB's */
  BOOT_DOT_CGB = 169,
  M_CYCLE_DOT = 1,   /* the CPU's M-cycles start on this dot of a line, and every 4 from it */
  DOT_PALETTES = 84, /* CGB: palette RAM goes to the PPU, until 2 dots after the HBlank interrupt */
  PALETTES_KEPT = 2,
  HDMA_AFTER_HBLANK = 1,        /* CGB: dots after the HBlank interrupt that an HBlank copy's block is asked for */
  CGB_C_PALETTE_WRITE_LEAD = 1, /* dots before the end of the M-cycle that a write to BGP, OBP0 or OBP1 lands, */
  CGB_E_PALETTE_WRITE_LEAD = 2, /* on a CGB C and on a CGB E */
  SPLIT_WRITE_LEAD = 2,  /* DMG: dots before the end of the M-cycle that a write to LCDC or a palette starts to land */
  SCROLL_WRITE_LEAD = 1, /* and that one to SCY or SCX lands, on any machine */
  DOCTOR_LY = 0x90,      /* Gameboy Doctor's logs were made with LY stuck here */
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

/* The fetcher's steps, a dot each: it reads a tile's index, then its row's low and high bits, and holds them until */
/* the background's pixels run out. */
enum {
  FETCH_TILE = 1,
  FETCH_LOW = 3,
  FETCH_HIGH = 5,
  FETCH_PUSH = 6,
};

/* An object's fetch, which stops the pixels while it goes on: what's to be done when the dots waited out are over. */
enum {
  OBJ_NONE,    /* no fetch going */
  OBJ_FETCHER, /* letting the background's fetcher get as far as its tile's low bits */
  OBJ_SCROLL,  /* an object at the left edge has waited out the scroll */
  OBJ_SETTLE,  /* its tile and flags are read from OAM */
  OBJ_LOW,     /* and where its row's low bits are worked out, */
  OBJ_LAST,    /* from here on, switching objects off doesn't stop it */
  OBJ_HIGH,    /* then its high bits', and the row goes into the objects' FIFO */
  OBJ_ABORTED, /* objects were switched off partway: back to the pixels */
};

enum {
  OBJ_SETTLE_DOTS = 3,
  SCROLL_PENALTY_MOST = 5,
  X_START = -16, /* a tile's worth of pixels to throw away, then the first tile's, of which the scroll's go too */
  X_SCROLLED = -TILE_SIZE,
  WX_LAST = 165, /* the last WX that starts a window */
  WX_OFF_SCREEN = 166,
  WINDOW_LINE_NONE = 0xFF, /* one before the window's first line */
  MAP_MASK = MAP_WIDTH - 1,
  TILE_ROW_MASK = TILE_SIZE - 1,
  FIFO_MASK = TILE_SIZE - 1,
  TILE_GLITCH_FROM = 0x80,
};

/* which mode's STAT interrupt is being asked for, which isn't always the mode STAT shows */
enum {
  IRQ_HBLANK = MODE_HBLANK,
  IRQ_VBLANK = MODE_VBLANK,
  IRQ_OAM = MODE_OAM_SCAN,
  IRQ_NONE,
};

enum {
  LY_NONE = -1, /* between lines, LY is compared with nothing */
};

/* the pixel that starts a window at WX 0, for each of the scroll's low 3 bits */
static const int8_t wx0_start[TILE_SIZE] = {
    -7, -1, -2, -3, -4, -5, -6, -6,
};

struct ppu {
  /* What shows. DMG: shades after the palettes, 0 (lightest) to 3. CGB: colors from the palettes. */
  uint16_t frame[LCD_HEIGHT][LCD_WIDTH];
  uint32_t frames;

  /* CGB */
  uint8_t vram_bank; /* the one the CPU sees */
  uint8_t bg_palettes[PALETTE_RAM_SIZE];
  uint8_t obj_palettes[PALETTE_RAM_SIZE];
  uint16_t hdma_source;
  uint16_t hdma_dest;  /* as written: its low 13 bits are the offset into VRAM */
  uint8_t hdma_blocks; /* left to copy, a block each HBlank */
  uint16_t hdma_run;   /* bytes the CPU is to be stopped for now */
  uint8_t hdma_first;  /* it has just taken the bus */
  uint8_t hdma_bus;    /* and found this on it */
  uint8_t hdma_status;

  /* where it's got to */
  uint16_t dot;        /* the next of the line's to run */
  uint8_t line;        /* which LY follows, but for the last line */
  uint8_t first_line;  /* the LCD's just come on: no OAM scan, and mode 0 until the drawing */
  uint8_t late;        /* dots that line's drawing is put back */
  uint8_t drawing;     /* the pixels are going */
  uint16_t hblank_dot; /* when the line's HBlank interrupt comes, the dot after its last pixel */

  /* STAT */
  int16_t ly_compare; /* what LYC is compared with: LY, a little behind it */
  uint8_t irq_mode;
  uint8_t lyc_line;  /* the comparison's side of the interrupt line */
  uint8_t stat_line; /* the enabled conditions OR'd together; the interrupt fires when this rises */

  /* what the PPU keeps from the CPU */
  uint8_t oam_read_locked;
  uint8_t oam_write_locked;
  uint8_t vram_read_locked;
  uint8_t vram_write_locked;
  uint8_t palettes_locked; /* CGB: palette RAM, while the pixels are going */

  /* a register write's second half, a dot after the first */
  uint8_t latch_dots;
  uint8_t latch_value;
  uint16_t latch_address;
  uint8_t wx_changed;  /* WX was written in the last dot */
  uint8_t tile_glitch; /* CGB: so was LCDC, clearing its tile data bit, which holds until the write's second half */

  /* the line's objects, rightmost first, and of those at one X the last in OAM first: fetched from the end */
  uint8_t objects;
  uint8_t object_x[OBJ_PER_LINE];
  uint8_t object_y[OBJ_PER_LINE];
  uint8_t object_index[OBJ_PER_LINE];

  /* the pixels */
  int16_t x;        /* the next to come out of the FIFO: negative ones are thrown away */
  uint8_t scrolled; /* SCX's low 3 bits, as many pixels as were thrown away for them */
  uint8_t fetch_step;
  uint8_t fetch_y; /* the row of the map the tile's index was read from */
  uint8_t tile;
  uint8_t tile_attr;
  uint8_t tile_low;
  uint8_t tile_high;
  uint8_t bg_low; /* the background's FIFO: a tile's row at a time, the next pixel in the top bits */
  uint8_t bg_high;
  uint8_t bg_attr;
  uint8_t bg_pixels;
  uint8_t obj_color[TILE_SIZE]; /* the objects' FIFO: a ring */
  uint8_t obj_attr[TILE_SIZE];
  uint8_t obj_priority[TILE_SIZE]; /* CGB: the object's place in OAM */
  uint8_t obj_head;
  uint8_t obj_pixels;
  uint8_t obj_step;
  uint8_t obj_wait;
  uint8_t obj_tile; /* of the object being fetched, from OAM */
  uint8_t obj_flags;
  uint16_t obj_low_address;
  uint8_t scroll_penalty; /* dots the first object at the left edge has to wait for the scroll */

  /* The window keeps its own line counter, which only moves on lines where the window is drawn. */
  uint8_t window;   /* the fetcher's on the window */
  uint8_t window_x; /* its next tile */
  uint8_t window_line;
  uint8_t window_fetching; /* none of its pixels are out yet */
  uint8_t window_late;     /* it starts next dot */
  uint8_t wy_triggered;    /* LY has matched WY this frame */
  uint8_t insert_pixel;    /* the window started again: a blank pixel goes in */
};

static struct ppu p;

const uint16_t *video_pixels(void) { return &p.frame[0][0]; }

uint32_t video_frames(void) { return p.frames; }

uint8_t vram_read(uint16_t address) { return vram[p.vram_bank * VRAM_SIZE + address - VRAM_START]; }

void vram_write(uint16_t address, uint8_t value) { vram[p.vram_bank * VRAM_SIZE + address - VRAM_START] = value; }

/* VRAM and OAM are the PPU's while it uses them: is this one of the CPU's accesses that doesn't get through? */
int video_locked(uint16_t address, int writing) {
  if (doctor) {
    return 0;
  }
  if (address >= VRAM_START && address < VRAM_START + VRAM_SIZE) {
    if (writing) {
      return p.vram_write_locked;
    }
    return p.vram_read_locked;
  }
  if (address >= OAM_START && address < OAM_START + OAM_SIZE) {
    if (writing) {
      return p.oam_write_locked;
    }
    return p.oam_read_locked;
  }
  return 0;
}

/* **************************************** */
/* Pixels */

static force_inline uint8_t shade(uint8_t palette, uint8_t color) {
  return (palette >> (color * SHADE_BITS)) & SHADE_MASK;
}

/* a tile row's byte mirrored, for a tile flipped in X */
static force_inline uint8_t reverse_bits(uint8_t bits) {
  bits = (bits & 0xF0) >> 4 | (bits & 0x0F) << 4;
  bits = (bits & 0xCC) >> 2 | (bits & 0x33) << 2;
  return (bits & 0xAA) >> 1 | (bits & 0x55) << 1;
}

/* the color index (0-3) in the top bits of a row's low and high bits */
static force_inline uint8_t top_pixel(uint8_t low, uint8_t high) { return (high >> 7) << 1 | low >> 7; }

/* CGB: color index of a palette, 0-7 in background palette RAM and 8-15 in object palette RAM */
static uint16_t palette_color(uint8_t palette, uint8_t index) {
  const uint8_t *ram = p.bg_palettes;
  if (palette >= PALETTES) {
    ram = p.obj_palettes;
    palette -= PALETTES;
  }
  const uint8_t *color = &ram[(palette * PALETTE_COLORS + index) * COLOR_BYTES];
  return (color[0] | color[1] << BYTE_BITS) & COLOR_MASK;
}

/* A background pixel and the object pixel over it (color 0 for none), as the LCD gets them: palettes and LCDC are */
/* read as the pixel goes by. */
static uint16_t mix(uint8_t bg_color, uint8_t bg_attr, uint8_t obj_color, uint8_t obj_attr) {
  uint8_t lcdc = IO_REG(IO_LCDC);
  uint8_t behind = bg_attr & ATTR_BEHIND_BG;
  int object = obj_color != 0 && (lcdc & LCDC_OBJ_ENABLE);
  if (object) {
    behind |= obj_attr & ATTR_BEHIND_BG;
  }
  if (!(lcdc & LCDC_BG_ENABLE)) {
    /* which takes the background's priority away from a CGB game, and blanks it for any other */
    if (cgb_mode) {
      behind = 0;
    } else {
      bg_color = 0;
    }
  }
  if (bg_color != 0 && behind) {
    object = 0;
  }

  if (cgb_mode) {
    if (object) {
      return palette_color(PALETTES + (obj_attr & ATTR_CGB_PALETTE), obj_color);
    }
    return palette_color(bg_attr & ATTR_CGB_PALETTE, bg_color);
  }
  /* A DMG game's color index goes through its palette register to a shade, which is what a DMG shows; a CGB */
  /* running it has palettes for those: 0 for the background, 8 for OBP0 and 9 for OBP1. */
  uint8_t pixel = shade(IO_REG(IO_BGP), bg_color);
  uint8_t palette = 0;
  if (object && (obj_attr & ATTR_PALETTE)) {
    pixel = shade(IO_REG(IO_OBP1), obj_color);
    palette = PALETTES + 1;
  } else if (object) {
    pixel = shade(IO_REG(IO_OBP0), obj_color);
    palette = PALETTES;
  }
  if (!cgb) {
    return pixel;
  }
  return palette_color(palette, pixel);
}

/* **************************************** */
/* CGB VRAM DMA, into the VRAM bank the CPU sees */

int video_dma_pending(void) { return p.hdma_run != 0; }

/* What the copy reads. It can't read VRAM while it writes it, nor anything from E000 up, and gets FF; but for the */
/* first byte after it takes the bus, which is what the CPU had there: the opcode it was about to run. */
static uint8_t hdma_read(void) {
  uint16_t source = p.hdma_source;
  if ((source >= VRAM_START && source < VRAM_START + VRAM_SIZE) || source >= ECHO_START) {
    if (p.hdma_first) {
      return p.hdma_bus;
    }
    return BYTE_MASK;
  }
  return mem_read(source);
}

/* the copy takes the bus, which has this on it */
void video_dma_begin(uint8_t bus) {
  p.hdma_first = 1;
  p.hdma_bus = bus;
}

/* a byte of the copy, the CPU being stopped for it */
void video_dma_byte(void) {
  uint8_t value = hdma_read();
  p.hdma_first = 0;
  /* lost while the PPU has VRAM, which is rougher than the hardware: there, one byte in four gets in between the */
  /* fetcher's reads */
  if (!p.vram_write_locked) {
    vram_write(VRAM_START + (p.hdma_dest & (VRAM_SIZE - 1)), value);
  }
  ++p.hdma_source;
  ++p.hdma_dest;
  if (--p.hdma_run % HDMA_BLOCK != 0) {
    return;
  }
  --p.hdma_blocks;
  p.hdma_status = p.hdma_blocks - 1;
  if (p.hdma_blocks == 0) {
    p.hdma_status = HDMA_IDLE;
  } else if (p.hdma_dest == 0) {
    /* The destination is all 16 bits of HDMA3 and HDMA4, of which the low 13 pick the byte of VRAM: it goes */
    /* round VRAM until it passes FFFF, and there the copy stops, with HDMA5 saying what was left. */
    p.hdma_status |= HDMA_HBLANK;
    p.hdma_blocks = 0;
    p.hdma_run = 0;
  }
}

/* the start of an HBlank: a block of an HBlank copy is asked for */
static void hdma_hblank(void) {
  if (p.hdma_blocks && !p.hdma_run) {
    p.hdma_run = HDMA_BLOCK;
  }
}

static void hdma_start(uint8_t value) {
  uint8_t blocks = (value & HDMA_BLOCKS) + 1;
  if (p.hdma_blocks && !(value & HDMA_HBLANK)) {
    /* ends an HBlank copy early, and HDMA5 reads back what was written */
    p.hdma_status = HDMA_HBLANK | value;
    p.hdma_blocks = 0;
  } else if (value & HDMA_HBLANK) {
    p.hdma_blocks = blocks;
    p.hdma_status = p.hdma_blocks - 1;
    /* with no line being drawn (in HBlank already, or the LCD off), the first block goes right away */
    if ((IO_REG(IO_STAT) & STAT_MODE) == MODE_HBLANK) {
      hdma_hblank();
    }
  } else {
    /* all at once, with the CPU stopped */
    p.hdma_blocks = blocks;
    p.hdma_status = p.hdma_blocks - 1;
    p.hdma_run = blocks * HDMA_BLOCK;
  }
}

/* **************************************** */
/* STAT, and its interrupt */

/* Where one line gives way to the next there are things a DMG shows for a dot or two: the LY=LYC flag dropping, */
/* mode 0 at the start of line 0, LY reading 0 early in line 153. A CGB C shows them too, to a CPU at the normal */
/* speed; one at double speed doesn't see them, and a CGB E hasn't got them. */
static force_inline int dmg_edges(void) { return !cgb || (!cgb_e && !double_speed); }

/* Compares LY with LYC, and works the interrupt line out again. With the LCD off nothing moves: the flag and the */
/* line stay as they were, so a comparison that still holds when it comes back on doesn't interrupt again. */
static void stat_update(void) {
  if (!(IO_REG(IO_LCDC) & LCDC_ENABLE)) {
    return;
  }
  uint8_t stat = IO_REG(IO_STAT);
  if (p.ly_compare != LY_NONE) {
    p.lyc_line = p.ly_compare == IO_REG(IO_LYC);
    stat &= ~STAT_LYC_EQUAL;
    if (p.lyc_line) {
      stat |= STAT_LYC_EQUAL;
    }
  } else {
    /* Between lines the flag drops, though the interrupt line holds. A CGB E keeps both, and so does a CGB C as */
    /* far as a CPU at double speed sees. */
    if (dmg_edges()) {
      stat &= ~STAT_LYC_EQUAL;
    }
  }
  IO_REG(IO_STAT) = stat;

  uint8_t line = 0;
  switch (p.irq_mode) {
    case IRQ_HBLANK:
      line = (stat & STAT_INT_HBLANK) != 0;
      break;
    case IRQ_VBLANK:
      line = (stat & STAT_INT_VBLANK) != 0;
      break;
    case IRQ_OAM:
      line = (stat & STAT_INT_OAM) != 0;
      break;
    default:
      break;
  }
  if ((stat & STAT_INT_LYC) && p.lyc_line) {
    line = 1;
  }
  if (line && !p.stat_line) {
    io_request(INT_STAT);
  }
  p.stat_line = line;
}

static void set_mode(uint8_t mode) { IO_REG(IO_STAT) = (IO_REG(IO_STAT) & ~STAT_MODE) | mode; }

static void lock(uint8_t locked) {
  p.oam_read_locked = locked;
  p.oam_write_locked = locked;
  p.vram_read_locked = locked;
  p.vram_write_locked = locked;
}

/* **************************************** */
/* Mode 2: the OAM scan, for the first 10 objects that cover the line */

/* The PPU's own read of OAM. OAM DMA takes OAM's address lines for the second half of each of its M-cycles, to */
/* write its byte, and a read by the PPU then gets the pair of bytes DMA is at, whichever it asked for: the first of */
/* them for an even address, the second for an odd one. */
static uint8_t oam_peek(int address) {
  int dma = io_dma_byte();
  if (dma >= 0 && ((p.dot - M_CYCLE_DOT) & (M_CYCLE - 1)) >= M_CYCLE / 2) {
    return oam[(dma & ~1) | (address & 1)];
  }
  return oam[address];
}

static void scan_object(int index) {
  uint8_t y = oam_peek(index * OBJ_BYTES + OBJ_Y);
  uint8_t x = oam_peek(index * OBJ_BYTES + OBJ_X);
  int height = TILE_SIZE;
  if (IO_REG(IO_LCDC) & LCDC_OBJ_TALL) {
    height = TILE_SIZE * 2;
  }
  int top = y - OBJ_Y_OFFSET;
  if (p.objects == OBJ_PER_LINE || p.line < top || p.line >= top + height) {
    return;
  }
  /* kept sorted, so that the leftmost, and of those the first in OAM, is at the end */
  int at = 0;
  while (at < p.objects && p.object_x[at] > x) {
    ++at;
  }
  for (int i = p.objects; i > at; --i) {
    p.object_x[i] = p.object_x[i - 1];
    p.object_y[i] = p.object_y[i - 1];
    p.object_index[i] = p.object_index[i - 1];
  }
  p.object_x[at] = x;
  p.object_y[at] = y;
  p.object_index[at] = index;
  ++p.objects;
}

/* **************************************** */
/* Mode 3: the background and window's fetcher */

/* the row of the tile map the fetcher is on */
static force_inline uint8_t fetch_row(void) {
  if (p.window) {
    return p.window_line;
  }
  return p.line + IO_REG(IO_SCY);
}

static void fetch_tile(void) {
  uint8_t lcdc = IO_REG(IO_LCDC);
  uint16_t map = TILE_MAP_0;
  uint8_t column;
  if (p.window) {
    if (lcdc & LCDC_WIN_MAP) {
      map = TILE_MAP_1;
    }
    column = p.window_x;
  } else {
    if (lcdc & LCDC_BG_MAP) {
      map = TILE_MAP_1;
    }
    if (p.x < X_SCROLLED) {
      column = IO_REG(IO_SCX) / TILE_SIZE;
    } else {
      /* SCX's high 5 bits are read for each tile; its low 3 only count as they were when the scroll was done */
      column = (IO_REG(IO_SCX) / TILE_SIZE + (p.scrolled + p.x + TILE_SIZE) / TILE_SIZE) & MAP_MASK;
    }
  }
  p.fetch_y = fetch_row();
  unsigned int cell = map - VRAM_START + (p.fetch_y / TILE_SIZE) * MAP_WIDTH + column;
  p.tile = vram[cell];
  /* A CGB's attributes are in VRAM bank 1 beside the map, where only a CGB game writes: 0 for any other. */
  p.tile_attr = vram[VRAM_SIZE + cell];
}

/* where the tile's row is in vram[]: LCDC and, off a CGB, the scroll are read again for each half */
static unsigned int tile_row_address(void) {
  int row = fetch_row() & TILE_ROW_MASK;
  if (p.tile_attr & ATTR_FLIP_Y) {
    row = TILE_SIZE - 1 - row;
  }
  unsigned int address = TILE_DATA_1 - VRAM_START + (int8_t)p.tile * TILE_BYTES;
  if (IO_REG(IO_LCDC) & LCDC_TILE_DATA) {
    address = TILE_DATA_0 - VRAM_START + p.tile * TILE_BYTES;
  }
  if (p.tile_attr & ATTR_BANK) {
    address += VRAM_SIZE;
  }
  return address + row * TILE_ROW_BYTES;
}

/* Half of the tile's row. CGB: in the dot after LCDC's tile data bit is cleared, a tile with an index under 128 gives
 */
/* the index itself for it. */
static uint8_t fetch_data(int high) {
  if (p.tile_glitch && p.tile < TILE_GLITCH_FROM) {
    return p.tile;
  }
  return vram[tile_row_address() + high];
}

/* the fetched row into the background's FIFO, once that's empty */
static void fetch_push(void) {
  if (p.bg_pixels) {
    return;
  }
  /* DMG: with the window switched off after its line has come, a blank pixel goes in where it would have started */
  if (p.wy_triggered && !(IO_REG(IO_LCDC) & LCDC_WIN_ENABLE) && !cgb && !p.window_fetching) {
    int wx = 0;
    if (p.x + WX_OFFSET >= 0) {
      wx = p.x + WX_OFFSET;
    }
    if (IO_REG(IO_WX) == wx) {
      p.bg_low = p.bg_high = p.bg_attr = 0;
      p.bg_pixels = 1;
      return;
    }
  }
  p.bg_low = p.tile_low;
  p.bg_high = p.tile_high;
  if (p.tile_attr & ATTR_FLIP_X) {
    p.bg_low = reverse_bits(p.bg_low);
    p.bg_high = reverse_bits(p.bg_high);
  }
  p.bg_attr = p.tile_attr;
  p.bg_pixels = TILE_SIZE;
  p.fetch_step = 0;
}

static void fetcher_dot(void) {
  switch (p.fetch_step) {
    case FETCH_TILE:
      fetch_tile();
      ++p.fetch_step;
      break;
    case FETCH_LOW:
      p.tile_low = fetch_data(0);
      ++p.fetch_step;
      break;
    case FETCH_HIGH:
      p.tile_high = fetch_data(1);
      if (p.window) {
        p.window_x = (p.window_x + 1) & MAP_MASK;
      }
      p.fetch_step = FETCH_PUSH;
      fetch_push();
      break;
    case FETCH_PUSH:
      fetch_push();
      break;
    default:
      ++p.fetch_step;
      break;
  }
}

/* **************************************** */
/* Mode 3: objects */

/* where the row of an object that covers this line is in vram[]: LCDC's object size is read each time */
static unsigned int object_row_address(void) {
  int rows = TILE_SIZE;
  uint8_t index = p.obj_tile;
  if (IO_REG(IO_LCDC) & LCDC_OBJ_TALL) {
    rows = TILE_SIZE * 2;
    index &= OBJ_TALL_TILE_MASK;
  }
  int row = (p.line - p.object_y[p.objects - 1]) & (rows - 1);
  if (p.obj_flags & ATTR_FLIP_Y) {
    row = rows - 1 - row;
  }
  unsigned int address = TILE_DATA_0 - VRAM_START + index * TILE_BYTES + row * TILE_ROW_BYTES;
  if (cgb_mode && (p.obj_flags & ATTR_BANK)) {
    address += VRAM_SIZE;
  }
  return address;
}

/* An object's row over the objects' FIFO: a pixel goes in where there's none yet, so the first fetched (the */
/* leftmost) wins, or in a CGB game where the one there is later in OAM. */
static void object_overlay(uint8_t low, uint8_t high, uint8_t attr, uint8_t index) {
  for (; p.obj_pixels < TILE_SIZE; ++p.obj_pixels) {
    p.obj_color[(p.obj_head + p.obj_pixels) & FIFO_MASK] = 0;
  }
  if (attr & ATTR_FLIP_X) {
    low = reverse_bits(low);
    high = reverse_bits(high);
  }
  uint8_t priority = 0;
  if (cgb_mode) {
    priority = index;
  }
  for (int i = 0; i < TILE_SIZE; ++i) {
    uint8_t color = top_pixel(low, high);
    int slot = (p.obj_head + i) & FIFO_MASK;
    if (color != 0 && (p.obj_color[slot] == 0 || p.obj_priority[slot] > priority)) {
      p.obj_color[slot] = color;
      p.obj_attr[slot] = attr;
      p.obj_priority[slot] = priority;
    }
    low <<= 1;
    high <<= 1;
  }
}

/* the X of an object that's to be fetched before the next pixel goes: ones at the left edge before any does */
static force_inline int object_match(void) {
  if (p.x < X_SCROLLED) {
    return 0;
  }
  return p.x + OBJ_X_OFFSET;
}

/* A DMG doesn't fetch objects at all while they're switched off; a CGB does, and hides them. */
static force_inline int objects_fetched(void) { return cgb || (IO_REG(IO_LCDC) & LCDC_OBJ_ENABLE); }

/* Is there an object to fetch here? Ones the pixels have gone past are dropped. */
static int object_due(void) {
  while (p.objects && p.x >= X_SCROLLED && p.object_x[p.objects - 1] < p.x + OBJ_X_OFFSET) {
    --p.objects;
  }
  return p.objects && objects_fetched() && p.object_x[p.objects - 1] == object_match();
}

/* A dot of the objects' fetches; returns whether one is holding the pixels up. A fetch waits for the background's */
/* fetcher to have its tile's low bits, then takes 6 dots; a second object at the same X follows straight on. */
static int objects_dot(void) {
  if (p.obj_step == OBJ_ABORTED) {
    p.obj_step = OBJ_NONE;
    return 0;
  }
  if (p.obj_wait && --p.obj_wait) {
    return 1;
  }
  for (;;) {
    switch (p.obj_step) {
      case OBJ_NONE:
        if (!object_due()) {
          return 0;
        }
        p.obj_step = OBJ_FETCHER;
        break;
      case OBJ_FETCHER:
        if (p.fetch_step < FETCH_HIGH || p.bg_pixels == 0) {
          fetcher_dot();
          p.obj_wait = 1;
          return 1;
        }
        p.obj_step = OBJ_SCROLL;
        /* the first at the left edge waits for the pixels the scroll throws away, too */
        if (p.scroll_penalty && p.object_x[p.objects - 1] == 0) {
          p.obj_wait = p.scroll_penalty;
          p.scroll_penalty = 0;
          return 1;
        }
        break;
      case OBJ_SCROLL:
        fetcher_dot();
        p.obj_wait = 1;
        p.obj_step = OBJ_SETTLE;
        return 1;
      case OBJ_SETTLE:
        fetcher_dot();
        /* the object's tile and flags are read from OAM here; its Y and X were kept from the scan */
        p.obj_tile = oam_peek(p.object_index[p.objects - 1] * OBJ_BYTES + OBJ_TILE);
        p.obj_flags = oam_peek(p.object_index[p.objects - 1] * OBJ_BYTES + OBJ_ATTR);
        p.obj_wait = OBJ_SETTLE_DOTS;
        p.obj_step = OBJ_LOW;
        return 1;
      case OBJ_LOW:
        p.obj_low_address = object_row_address();
        p.obj_wait = 1;
        p.obj_step = OBJ_LAST;
        return 1;
      case OBJ_LAST:
        p.obj_wait = 1;
        p.obj_step = OBJ_HIGH;
        return 1;
      case OBJ_HIGH:
        /* the high bits' address is worked out again, so a change of object size in between splits the row */
        object_overlay(vram[p.obj_low_address], vram[object_row_address() + 1], p.obj_flags,
                       p.object_index[p.objects - 1]);
        --p.objects;
        p.obj_step = OBJ_NONE;
        break;
      default:
        return 0;
    }
  }
}

/* **************************************** */
/* Mode 3: the window, and the pixels */

static void window_start(void) {
  p.window = 1;
  p.window_x = 0;
  p.window_fetching = 1;
  p.bg_pixels = 0;
  p.fetch_step = 0;
}

/* Does the window start at the next pixel? Returns whether it does, but only from the dot after. */
static int window_check(void) {
  uint8_t lcdc = IO_REG(IO_LCDC);
  uint8_t wx = IO_REG(IO_WX);
  if (!p.window && p.wy_triggered && (lcdc & LCDC_WIN_ENABLE)) {
    int start = 0;
    if (wx == 0) {
      start = p.x == wx0_start[IO_REG(IO_SCX) & TILE_ROW_MASK] && !p.wx_changed;
    } else if (wx <= WX_LAST + cgb) {
      if (wx == p.x + WX_OFFSET) {
        start = 1;
      } else if (wx == p.x + WX_OFFSET - 1 && !p.wx_changed) {
        /* A pixel late for the comparison (the window only just switched on, say), it still starts. On some DMGs */
        /* the LCD then loses step with the PPU by a pixel; not on the ones mealybug's pictures are from. */
        start = 1;
      }
    }
    if (start) {
      ++p.window_line;
      if (wx == 0 && (IO_REG(IO_SCX) & TILE_ROW_MASK)) {
        p.window_late = 1;
        return 1;
      }
      window_start();
    } else if (!cgb && wx == WX_OFF_SCREEN && wx == p.x + WX_OFFSET) {
      /* DMG: too far right to show, but its line counter moves */
      ++p.window_line;
    }
  }
  /* DMG: WX matching again while the window's being drawn puts a blank pixel in */
  if (!cgb && p.window && !p.window_fetching && p.fetch_step == 0 && wx == p.x + WX_OFFSET &&
      p.bg_pixels == TILE_SIZE) {
    p.insert_pixel = 1;
  }
  return 0;
}

/* The next pixel out of the FIFOs, if the background's has one: thrown away while x is negative, else to the LCD. */
static void pixel_pop(void) {
  /* an object at the left edge still to fetch holds the pixels back, and the scroll with them */
  if (p.objects && objects_fetched() && p.object_x[p.objects - 1] == 0) {
    return;
  }
  if (p.bg_pixels == 0) {
    return;
  }
  uint8_t color = 0;
  uint8_t attr = 0;
  if (p.insert_pixel) {
    p.insert_pixel = 0;
  } else {
    color = top_pixel(p.bg_low, p.bg_high);
    attr = p.bg_attr;
    p.bg_low <<= 1;
    p.bg_high <<= 1;
    --p.bg_pixels;
  }
  uint8_t obj_color = 0;
  uint8_t obj_attr = 0;
  if (p.obj_pixels) {
    obj_color = p.obj_color[p.obj_head];
    obj_attr = p.obj_attr[p.obj_head];
    p.obj_head = (p.obj_head + 1) & FIFO_MASK;
    --p.obj_pixels;
  }

  /* The scroll's low 3 bits: pixels go until x's match them, then the rest of a tile's worth. If SCX moves so that */
  /* they never match, it goes round again. */
  if (p.x < X_SCROLLED) {
    uint8_t scroll = IO_REG(IO_SCX) & TILE_ROW_MASK;
    if ((p.x & TILE_ROW_MASK) == scroll) {
      p.x = X_SCROLLED;
      p.scrolled = scroll;
    } else if (p.window_fetching && (p.x & TILE_ROW_MASK) == TILE_ROW_MASK - 1 && scroll == TILE_ROW_MASK) {
      p.x = X_SCROLLED;
      p.scrolled = scroll;
    } else if (p.x == X_SCROLLED - 1) {
      p.x = X_START;
      return;
    }
  }
  if (p.x < 0) {
    ++p.x;
    return;
  }
  p.frame[p.line][p.x] = mix(color, attr, obj_color, obj_attr);
  ++p.x;
  p.window_fetching = 0;
}

static void hblank_mode(void) {
  set_mode(MODE_HBLANK);
  lock(0);
  p.irq_mode = IRQ_HBLANK;
}

/* mode 3 is over, as long as the scroll, the window and the objects made it */
static void drawing_end(void) {
  p.drawing = 0;
  /* the interrupt comes a dot after the mode; at double speed the mode waits for it */
  if (!double_speed) {
    hblank_mode();
  }
  p.hblank_dot = p.dot + 1;
}

static void pixels_start(void) {
  if ((IO_REG(IO_LCDC) & LCDC_WIN_ENABLE) && IO_REG(IO_WY) == p.line) {
    p.wy_triggered = 1;
  }
  p.drawing = 1;
  p.x = X_START;
  p.scrolled = 0;
  p.fetch_step = 0;
  /* a tile's worth of nothing to start with, which only takes time */
  p.bg_low = p.bg_high = p.bg_attr = 0;
  p.bg_pixels = TILE_SIZE;
  p.obj_head = 0;
  p.obj_pixels = 0;
  p.obj_step = OBJ_NONE;
  p.obj_wait = 0;
  p.scroll_penalty = IO_REG(IO_SCX) & TILE_ROW_MASK;
  if (p.scroll_penalty > SCROLL_PENALTY_MOST) {
    p.scroll_penalty = SCROLL_PENALTY_MOST;
  }
  p.window = 0;
  p.window_fetching = 0;
  p.window_late = 0;
  p.insert_pixel = 0;
}

static void pixels_dot(void) {
  if (p.window_late) {
    p.window_late = 0;
    window_start();
  } else if (p.obj_step == OBJ_NONE && window_check()) {
    return;
  }
  if (objects_dot()) {
    return;
  }
  pixel_pop();
  fetcher_dot();
  if (p.x == LCD_WIDTH) {
    drawing_end();
  }
}

/* **************************************** */
/* The line */

/* a line on the screen, bar its pixels */
static void line_dot(void) {
  uint16_t d = p.dot;
  if (p.hblank_dot != NO_DOT && d == p.hblank_dot + HDMA_AFTER_HBLANK) {
    hdma_hblank();
  }
  if (p.first_line) {
    if (d == DOT_DRAWING + p.late - DOT_SCAN_EACH) {
      p.oam_write_locked = 1;
    }
  } else if (d == DOT_LINE) {
    IO_REG(IO_LY) = p.line;
    p.oam_read_locked = 1;
    p.oam_write_locked = cgb;
    p.hblank_dot = NO_DOT;
    /* The OAM interrupt comes a dot before the mode, but not on the first line; LY isn't compared for that dot. */
    if (p.line != 0) {
      p.ly_compare = LY_NONE;
      p.irq_mode = IRQ_OAM;
      set_mode(MODE_HBLANK);
    } else if (dmg_edges()) {
      set_mode(MODE_HBLANK);
    }
    stat_update();
  } else if (d == DOT_SCAN) {
    set_mode(MODE_OAM_SCAN);
    p.oam_write_locked = 1;
    p.ly_compare = p.line;
    p.irq_mode = IRQ_OAM;
    stat_update();
    /* asked for only for that dot */
    p.irq_mode = IRQ_NONE;
    stat_update();
    p.objects = 0;
  } else if (d >= DOT_SCAN_FIRST && d <= DOT_DRAWING && (d - DOT_SCAN_FIRST) % DOT_SCAN_EACH == 0) {
    scan_object((d - DOT_SCAN_FIRST) / DOT_SCAN_EACH);
    if (d == DOT_SCAN_RELEASE && !cgb) {
      /* a DMG stops reading VRAM a little before the drawing, and lets OAM be written until it */
      p.vram_read_locked = 1;
      p.oam_write_locked = 0;
    }
  }

  if (d == DOT_DRAWING + p.late) {
    set_mode(MODE_DRAWING);
    p.irq_mode = IRQ_NONE;
    lock(1);
    stat_update();
  } else if (d == DOT_PALETTES + p.late) {
    p.palettes_locked = cgb;
  } else if (d == DOT_PIXELS + p.late) {
    pixels_start();
    pixels_dot();
  } else if (d == p.hblank_dot + PALETTES_KEPT) {
    p.palettes_locked = 0;
  } else if (d == p.hblank_dot) {
    hblank_mode();
    stat_update();
  } else if (d == DOT_WINDOW_Y) {
    if ((IO_REG(IO_LCDC) & LCDC_WIN_ENABLE) && IO_REG(IO_WY) == p.line) {
      p.wy_triggered = 1;
    }
  }
}

/* the 10 lines of VBlank; the last shows LY as 0 almost from its start */
static void vblank_dot(void) {
  uint16_t d = p.dot;
  if (d == DOT_LINE) {
    IO_REG(IO_LY) = p.line;
    p.ly_compare = LY_NONE;
    stat_update();
    /* STAT's OAM interrupt comes for the first line of VBlank, too */
    if (p.line == LCD_HEIGHT && !p.stat_line && (IO_REG(IO_STAT) & STAT_INT_OAM)) {
      io_request(INT_STAT);
    }
    return;
  }
  if (p.line == LAST_LINE) {
    /* A DMG has LY read 0 and starts comparing 153 together. A CGB compares from where any VBlank line does, and */
    /* reads 0 two dots later than a DMG unless it shows a DMG's edges. Then all but a CGB E compare nothing for a */
    /* while, before they compare 0. */
    int zero_dot = DOT_LAST_ZERO;
    int compare_dot = DOT_LAST_ZERO;
    if (!dmg_edges()) {
      zero_dot = DOT_LAST_NONE;
    }
    if (cgb) {
      compare_dot = DOT_VBLANK_COMPARE;
    }
    if (d == zero_dot) {
      IO_REG(IO_LY) = 0;
    }
    if (d == compare_dot) {
      p.ly_compare = LAST_LINE;
      stat_update();
    }
    if (d == DOT_LAST_NONE && !cgb_e) {
      p.ly_compare = LY_NONE;
      stat_update();
    }
    if (d == DOT_LAST_COMPARE) {
      p.ly_compare = 0;
      stat_update();
    }
    return;
  }
  if (d == DOT_VBLANK_COMPARE) {
    p.ly_compare = p.line;
    stat_update();
  } else if (d == DOT_VBLANK && p.line == LCD_HEIGHT) {
    set_mode(MODE_VBLANK);
    io_request(INT_VBLANK);
    p.irq_mode = IRQ_OAM;
    stat_update();
    p.irq_mode = IRQ_VBLANK;
    stat_update();
    ++p.frames;
    host_frame();
  }
}

static void dot(void) {
  if (p.drawing) {
    pixels_dot();
  } else if (p.line < LCD_HEIGHT) {
    line_dot();
  } else {
    vblank_dot();
  }
  if (++p.dot < LINE_CYCLES) {
    return;
  }
  p.dot = 0;
  p.first_line = 0;
  p.late = 0;
  if (p.drawing) {
    /* never, short of a line with more to fetch than there's time for */
    drawing_end();
  }
  if (++p.line == LCD_LINES) {
    p.line = 0;
    p.window_line = WINDOW_LINE_NONE;
    p.wy_triggered = 0;
  }
}

static void register_set(uint16_t address, uint8_t value, int lead);

void video_tick(uint32_t cycles) {
  for (; cycles > 0; --cycles) {
    if (p.latch_dots && --p.latch_dots == 0) {
      p.tile_glitch = 0;
      register_set(p.latch_address, p.latch_value, 1);
    }
    if (IO_REG(IO_LCDC) & LCDC_ENABLE) {
      dot();
    }
    p.wx_changed = 0;
  }
}

/* the whole screen one shade, on a DMG, or one color */
static void fill(uint8_t shade, uint16_t color) {
  uint16_t value = color;
  if (!cgb) {
    value = shade;
  }
  for (int y = 0; y < LCD_HEIGHT; ++y) {
    for (int x = 0; x < LCD_WIDTH; ++x) {
      p.frame[y][x] = value;
    }
  }
}

static void video_blank(void) { fill(0, WHITE); }

/* The clock's stopped: the DMG's screen goes blank, and a CGB's black, its PPU running on without VRAM; unless it */
/* was drawing, when it has VRAM still, and the picture stays. */
void video_stop(void) {
  if (!cgb) {
    video_blank();
  } else if ((IO_REG(IO_STAT) & STAT_MODE) != MODE_DRAWING) {
    fill(0, BLACK);
  }
}

/* The LCD comes on partway into a line 0 that has no OAM scan and shows mode 0 until its drawing, which is late; */
/* lead is how many dots of the write's M-cycle are still to run. */
static void lcd_on(int lead) {
  p.line = 0;
  p.dot = FIRST_LINE_DOT - lead;
  p.first_line = 1;
  p.late = FIRST_LINE_LATE;
  p.hblank_dot = NO_DOT;
  p.objects = 0;
  p.ly_compare = 0;
  p.irq_mode = IRQ_NONE;
  stat_update();
}

static void lcd_off(void) {
  video_blank();
  IO_REG(IO_LY) = 0;
  set_mode(MODE_HBLANK);
  lock(0);
  p.palettes_locked = 0;
  p.line = 0;
  p.drawing = 0;
  p.window_line = WINDOW_LINE_NONE;
  p.wy_triggered = 0;
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

void video_reset(void) {
  memset(&p, 0, sizeof(p));
  p.hdma_status = HDMA_IDLE;
  p.hblank_dot = NO_DOT;
  p.irq_mode = IRQ_NONE;
  p.window_line = WINDOW_LINE_NONE;
}

/* The screen as the boot ROM leaves it, with the LCD on and in VBlank: a DMG near the end of its last line, where */
/* LY already reads 0, and a CGB a third of the way along its first. */
void video_bios_init(void) {
  IO_REG(IO_LCDC) = LCDC_ENABLE;
  IO_REG(IO_STAT) = MODE_VBLANK;
  p.irq_mode = IRQ_VBLANK;
  p.line = BOOT_LINE_DMG;
  p.dot = BOOT_DOT_DMG;
  if (cgb) {
    p.line = BOOT_LINE_CGB;
    p.dot = BOOT_DOT_CGB;
    IO_REG(IO_LY) = BOOT_LINE_CGB;
  }
  p.ly_compare = IO_REG(IO_LY);
  stat_update();
  if (!cgb) {
    load_logo();
    return;
  }
  memset(p.bg_palettes, BYTE_MASK, sizeof(p.bg_palettes)); /* white */
  memset(p.obj_palettes, BYTE_MASK, sizeof(p.obj_palettes));
  if (cgb && !cgb_mode) {
    set_palette(p.bg_palettes, 0, compat_background);
    set_palette(p.obj_palettes, 0, compat_objects);
    set_palette(p.obj_palettes, 1, compat_objects);
  }
  video_blank();
}

/* **************************************** */
/* Registers FF40-FF4B (bar DMA), and a CGB game's VBK, HDMA1-5, BCPS/BCPD, OCPS/OCPD and OPRI */

/* BCPD/OCPD: the byte of palette RAM that BCPS/OCPS (spec) points at, which can then move on */
static void palette_write(uint8_t *ram, uint16_t spec, uint8_t value) {
  uint8_t index = IO_REG(spec) & PALETTE_INDEX;
  /* while the PPU has the palettes the write is lost, but the index still moves on */
  if (!p.palettes_locked) {
    ram[index] = value;
  }
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
      /* Read on the very dot it changes, LY is the old line and the new, ANDed. A CPU only gets there once a speed */
      /* switch has put its M-cycles out of step with the lines. */
      if ((IO_REG(IO_LCDC) & LCDC_ENABLE) && p.dot == DOT_LINE && !p.first_line) {
        return IO_REG(IO_LY) & p.line;
      }
      return IO_REG(IO_LY);
    case IO_VBK:
      return VBK_UNUSED | p.vram_bank;
    case IO_HDMA1:
    case IO_HDMA2:
    case IO_HDMA3:
    case IO_HDMA4:
      return BYTE_MASK; /* write-only */
    case IO_HDMA5:
      return p.hdma_status;
    case IO_BCPS:
    case IO_OCPS:
      return IO_REG(address) | PALETTE_SPEC_UNUSED;
    case IO_BCPD:
      if (p.palettes_locked) {
        return BYTE_MASK;
      }
      return p.bg_palettes[IO_REG(IO_BCPS) & PALETTE_INDEX];
    case IO_OCPD:
      if (p.palettes_locked) {
        return BYTE_MASK;
      }
      return p.obj_palettes[IO_REG(IO_OCPS) & PALETTE_INDEX];
    case IO_OPRI:
      return IO_REG(IO_OPRI) | OPRI_UNUSED;
    default:
      return IO_REG(address);
  }
}

static void lcdc_set(uint8_t value, int lead) {
  uint8_t old = IO_REG(IO_LCDC);
  IO_REG(IO_LCDC) = value;
  if ((old ^ value) & LCDC_ENABLE) {
    if (value & LCDC_ENABLE) {
      lcd_on(lead);
    } else {
      lcd_off();
    }
  }
  if (!(value & LCDC_WIN_ENABLE)) {
    p.window = 0;
  }
}

/* A register takes its new value, with lead dots of the write's M-cycle still to run. */
static void register_set(uint16_t address, uint8_t value, int lead) {
  switch (address) {
    case IO_LCDC:
      lcdc_set(value, lead);
      break;
    case IO_STAT:
      IO_REG(IO_STAT) = (IO_REG(IO_STAT) & ~STAT_WRITABLE) | (value & STAT_WRITABLE);
      stat_update();
      break;
    case IO_LY:
      /* read-only */
      break;
    case IO_LYC:
      IO_REG(IO_LYC) = value;
      stat_update();
      break;
    case IO_WX:
      IO_REG(IO_WX) = value;
      p.wx_changed = 1;
      break;
    case IO_VBK:
      p.vram_bank = value & 1;
      break;
    case IO_HDMA1:
      p.hdma_source = (p.hdma_source & BYTE_MASK) | value << BYTE_BITS;
      break;
    case IO_HDMA2:
      p.hdma_source = (p.hdma_source & ~BYTE_MASK) | (value & HDMA_SOURCE);
      break;
    case IO_HDMA3:
      p.hdma_dest = (p.hdma_dest & BYTE_MASK) | value << BYTE_BITS;
      break;
    case IO_HDMA4:
      p.hdma_dest = (p.hdma_dest & ~BYTE_MASK) | (value & HDMA_SOURCE);
      break;
    case IO_HDMA5:
      hdma_start(value);
      break;
    case IO_BCPD:
      palette_write(p.bg_palettes, IO_BCPS, value);
      break;
    case IO_OCPD:
      palette_write(p.obj_palettes, IO_OCPS, value);
      break;
    default:
      IO_REG(address) = value;
      break;
  }
}

/* A DMG's registers that the pixels go through take a write in two halves, a dot apart. The palettes are the old */
/* and new values OR'd together for the first. LCDC waits for the second, but an object's fetch sees objects switched */
/* off in the first, and with the first pixel about to go, so do the pixels, for objects and the background both. */
static int write_is_split(uint16_t address) {
  return !cgb && (address == IO_LCDC || address == IO_BGP || address == IO_OBP0 || address == IO_OBP1);
}

/* How many dots before the end of its M-cycle a write to this register lands: the bus holds that many back. */
uint32_t video_write_lead(uint16_t address) {
  if (cgb) {
    if (address == IO_BGP || address == IO_OBP0 || address == IO_OBP1) {
      if (cgb_e) {
        return CGB_E_PALETTE_WRITE_LEAD;
      }
      return CGB_C_PALETTE_WRITE_LEAD;
    }
  } else if (write_is_split(address)) {
    return SPLIT_WRITE_LEAD;
  }
  if (address == IO_SCY || address == IO_SCX) {
    return SCROLL_WRITE_LEAD;
  }
  return 0;
}

void video_write(uint16_t address, uint8_t value) {
  if (doctor) {
    /* the PPU isn't run, so there's no second half to come */
    register_set(address, value, 0);
    return;
  }
  if (p.latch_dots) {
    /* writes can't really come this close together: finish the last */
    p.latch_dots = 0;
    p.tile_glitch = 0;
    register_set(p.latch_address, p.latch_value, 0);
  }
  uint8_t old = IO_REG(address);
  uint8_t first = old | value;
  int lead = SPLIT_WRITE_LEAD;
  if (cgb && address == IO_LCDC && (old & ~value & LCDC_TILE_DATA)) {
    /* CGB: the rest of LCDC changes now, at the end of the M-cycle, and the tile data bit a dot after */
    first = value | LCDC_TILE_DATA;
    lead = 0;
    p.tile_glitch = 1;
  } else if (!write_is_split(address)) {
    register_set(address, value, 0);
    return;
  } else if (address == IO_LCDC) {
    first = old;
    if (p.drawing && p.x == 0) {
      first &= value | ~(LCDC_OBJ_ENABLE | LCDC_BG_ENABLE);
    }
    /* objects switched off partway through fetching one, which stops there */
    if ((old & ~value & LCDC_OBJ_ENABLE) && p.drawing && p.obj_step != OBJ_NONE && p.obj_step != OBJ_HIGH) {
      p.obj_step = OBJ_ABORTED;
      p.obj_wait = 0;
    }
  }
  register_set(address, first, lead);
  p.latch_address = address;
  p.latch_value = value;
  p.latch_dots = SPLIT_WRITE_LEAD;
}
