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

#ifndef _FAILBOY_H_
#define _FAILBOY_H_

#include <stdint.h>

#if !defined(NDEBUG)
// Debug builds: plain functions, so they can be stepped into
#define force_inline
#elif defined(_MSC_VER)
// Microsoft Visual C/C++
#define force_inline __forceinline
#elif defined(__GNUC__) || defined(__clang__)
// GCC, Clang, and compatible compilers
#define force_inline inline __attribute__((always_inline))
#else
// Fallback for other compilers
#define force_inline inline
#endif

typedef uint8_t (*read_f)(uint16_t);
typedef void (*write_f)(uint16_t, uint8_t);

/* Timings are counted in clock (T) cycles; the CPU works in machine cycles of 4. */
enum {
  M_CYCLE = 4,
  M_CYCLE_SHL = 2,  // left shift for converting M-cycles to T-cycles
  CLOCK_HZ = 4194304,
};

/* failboy.c */
extern int doctor; /* Gameboy Doctor trace mode */

/* cart.c */
int cart_load(const char *);
void cart_free(void);
unsigned int cart_battery_ram_size(void);                   /* 0 unless the cartridge RAM has a battery */
void cart_read_battery_ram(uint8_t *, unsigned int);        /* copies the battery RAM out, to save it */
void cart_write_battery_ram(const uint8_t *, unsigned int); /* copies a saved battery RAM back in */

/* the MBC3's clock */
enum {
  CART_RTC_SIZE = 13, /* bytes of clock state, for cart_read_rtc() and cart_write_rtc() */
};

int cart_has_rtc(void);
void cart_read_rtc(uint8_t *, unsigned int);        /* copies the clock's state out, to save it */
void cart_write_rtc(const uint8_t *, unsigned int); /* restores a saved clock, which catches up on the time since */

/* io.c */
enum {
  IO_P1 = 0xFF00,
  IO_SB = 0xFF01,
  IO_SC = 0xFF02,
  IO_DIV = 0xFF04,
  IO_TIMA = 0xFF05,
  IO_TMA = 0xFF06,
  IO_TAC = 0xFF07,
  IO_IF = 0xFF0F,
  IO_LCDC = 0xFF40,
  IO_STAT = 0xFF41,
  IO_SCY = 0xFF42,
  IO_SCX = 0xFF43,
  IO_LY = 0xFF44,
  IO_LYC = 0xFF45,
  IO_DMA = 0xFF46,
  IO_BGP = 0xFF47,
  IO_OBP0 = 0xFF48,
  IO_OBP1 = 0xFF49,
  IO_WY = 0xFF4A,
  IO_WX = 0xFF4B,
  IO_IE = 0xFFFF,
  IO_SIZE = 0x80, /* registers FF00-FF7F */
};

/* IF/IE bits, highest priority first */
enum {
  INT_VBLANK = 0x01,
  INT_STAT = 0x02,
  INT_TIMER = 0x04,
  INT_SERIAL = 0x08,
  INT_JOYPAD = 0x10,
  INT_ALL = 0x1F,
};

/* what Blargg's test ROMs have reported over the serial port */
enum {
  SERIAL_NONE,
  SERIAL_PASSED,
  SERIAL_FAILED,
};

/* buttons for io_joypad(); a set bit means pressed */
enum {
  JOYPAD_RIGHT = 0x01,
  JOYPAD_LEFT = 0x02,
  JOYPAD_UP = 0x04,
  JOYPAD_DOWN = 0x08,
  JOYPAD_A = 0x10,
  JOYPAD_B = 0x20,
  JOYPAD_SELECT = 0x40,
  JOYPAD_START = 0x80,
};

extern uint8_t io_reg[IO_SIZE]; /* FF00-FF7F */
extern uint8_t io_ie;           /* FFFF */
#define IO_REG(address) (io_reg[(address) & (IO_SIZE - 1)])

void io_request(uint8_t);
void io_tick(uint32_t);
void io_joypad(uint8_t);
int io_serial_result(void);

/* video.c */
enum {
  LCD_WIDTH = 160,
  LCD_HEIGHT = 144,
  LCD_LINES = 154,   /* per frame, including the 10 of VBlank */
  LINE_CYCLES = 456, /* T-cycles per line */
  FRAME_CYCLES = LCD_LINES * LINE_CYCLES,
};

void video_tick(uint32_t);
uint8_t video_read(uint16_t);
void video_write(uint16_t, uint8_t);
const uint8_t *video_framebuffer(void); /* LCD_WIDTH * LCD_HEIGHT shades, 0 (lightest) to 3 (darkest) */
uint32_t video_frames(void);            /* frames finished so far; one more each time VBlank starts */

/* audio.c */
enum {
  AUDIO_START = 0xFF10,                        /* NR10, the first sound register */
  AUDIO_END = 0xFF3F,                          /* the last byte of wave RAM */
  AUDIO_SAMPLE_CYCLES = 88,                    /* T-cycles per output sample */
  AUDIO_RATE = CLOCK_HZ / AUDIO_SAMPLE_CYCLES, /* stereo samples a second, 47662 (and a half) */
};

void audio_bios_init(void);
void audio_tick(uint32_t);
void audio_sequencer_clock(void); /* the frame sequencer's 512 Hz clock, which comes off DIV */
uint8_t audio_read(uint16_t);
void audio_write(uint16_t, uint8_t);
const int16_t *audio_samples(unsigned int *); /* the samples (left, right) made since the last call, and how many */

/* mem.c */
enum {
  VRAM_START = 0x8000,
  VRAM_SIZE = 0x2000,
  WRAM_START = 0xC000,
  WRAM_SIZE = 0x2000,
  ECHO_START = 0xE000, /* mirror of WRAM */
  OAM_START = 0xFE00,
  OAM_SIZE = 0xA0,
  HRAM_START = 0xFF80,
  HRAM_SIZE = 0x7F,
};

int mem_alloc(void);
void mem_free(void);

extern uint8_t *oam;
extern uint8_t *vram;

uint8_t mem_read(uint16_t);
uint16_t mem_read16(uint16_t);

void mem_write(uint16_t, uint8_t);
void mem_write16(uint16_t, uint16_t);

/* cpu.c */
struct registers {
  uint16_t PC;

  union {
    uint16_t SP;

    struct {
      uint8_t SPLO;
      uint8_t SPHI;
    };
  };

  union {
    uint16_t AF;

    struct {
      union {
        uint8_t F;

        struct {
          uint8_t : 4;     /* 0-3 unused */
          uint8_t F_C : 1; /* carry */
          uint8_t F_H : 1; /* half-carry (BCD) */
          uint8_t F_N : 1; /* add/sub flag (BCD) */
          uint8_t F_Z : 1; /* zero flag */
        };
      };

      uint8_t A;
    };
  };

  union {
    uint16_t BC;

    struct {
      uint8_t C;
      uint8_t B;
    };
  };

  union {
    uint16_t DE;

    struct {
      uint8_t E;
      uint8_t D;
    };
  };

  union {
    uint16_t HL;

    struct {
      uint8_t L;
      uint8_t H;
    };
  };
};

extern struct registers r;
extern uint64_t cycle_counter;

extern uint8_t ime;
extern uint8_t ei_delay;
extern uint8_t halted;
extern uint8_t halt_bug;
extern uint8_t cpu_locked;

void cpu_bios_init(void);
void cpu_trace(void);
uint32_t step(void); /* runs one instruction (or interrupt/halt cycle), returns T-cycles */

#endif /* _FAILBOY_H_ */
