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

#ifndef _CORE_H_
#define _CORE_H_

#include <stdint.h>

#include "failboy.h"

/* What the core's modules share with each other. Hosts use failboy.h, and nothing from here. */

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
  CLOCK_HZ = GB_CLOCK_HZ,
};

/* gb.c */
extern int doctor; /* being as Gameboy Doctor's logs expect */

extern int revision;  /* the machine emulated: a gb_hardware */
extern uint8_t cgb;   /* it's a Game Boy Color */
extern uint8_t cgb_d; /* and its CPU is a CGB D, not a CGB C */
extern uint8_t
    cgb_mode; /* and running a color game, so all it adds is switched on; if not, it's in compatibility mode */

/* the host's callbacks, each doing nothing if the host left it out */
void host_frame(void);                         /* a frame is finished */
int host_audio(const int16_t *, unsigned int); /* the sample buffer is full; returns whether the host took it */
void host_serial(uint8_t);                     /* a byte sent out of the link port */
void host_trace(void);                         /* the CPU has finished an instruction */
int64_t host_clock(void);                      /* Unix time, or 0 */

/* cart.c */
enum gb_error cart_load(const uint8_t *, uint32_t); /* the ROM stays the caller's */
void cart_free(void);
void cart_reset(int); /* the mapper as the power coming on leaves it; and if set, RAM with no battery cleared */
uint8_t *cart_ram(uint32_t *);                       /* all of the cartridge's RAM, and its size; NULL if it has none */
int cart_cgb(void);                                  /* the header says the game can use a Game Boy Color */
unsigned int cart_battery_ram_size(void);            /* 0 unless the cartridge RAM has a battery */
void cart_read_battery_ram(uint8_t *, unsigned int); /* copies the battery RAM out, to save it */
void cart_write_battery_ram(const uint8_t *, unsigned int); /* copies a saved battery RAM back in */

/* the MBC3's clock */
enum {
  CART_RTC_SIZE = GB_RTC_SIZE, /* bytes of clock state, for cart_read_rtc() and cart_write_rtc() */
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
  IO_KEY1 = 0xFF4D,  /* CGB: speed switch */
  IO_VBK = 0xFF4F,   /* CGB: VRAM bank */
  IO_HDMA1 = 0xFF51, /* CGB: VRAM DMA source (high, low), destination (high, low), then length and mode */
  IO_HDMA2 = 0xFF52,
  IO_HDMA3 = 0xFF53,
  IO_HDMA4 = 0xFF54,
  IO_HDMA5 = 0xFF55,
  IO_BCPS = 0xFF68, /* CGB: background palette index, then data */
  IO_BCPD = 0xFF69,
  IO_OCPS = 0xFF6A, /* CGB: object palette index, then data */
  IO_OCPD = 0xFF6B,
  IO_OPRI = 0xFF6C,  /* CGB: object priority mode */
  IO_SVBK = 0xFF70,  /* CGB: WRAM bank */
  IO_PCM12 = 0xFF76, /* CGB: the outputs of sound channels 1 and 2, then 3 and 4 */
  IO_PCM34 = 0xFF77,
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

extern uint8_t io_reg[IO_SIZE]; /* FF00-FF7F */
extern uint8_t io_ie;           /* FFFF */
#define IO_REG(address) (io_reg[(address) & (IO_SIZE - 1)])

void io_reset(void);     /* as the power coming on leaves it; the same for the other modules' */
void io_bios_init(void); /* the divider as the boot ROM leaves it */
void io_request(uint8_t);
void io_tick(uint32_t);
void io_joypad(uint8_t);       /* the GB_BUTTON_s held down */
int io_joypad_pressed(void);   /* one of them on a line P1 has selected */
int io_speed_armed(void);      /* CGB: KEY1 asks for a speed switch */
int io_speed_switch(int late); /* CGB: STOP switching speed, if KEY1 asks; returns whether it did */
int io_dma_conflict(uint16_t); /* the CPU's access collides with OAM DMA: it's to OAM, or the bus DMA is reading */
uint8_t io_dma_read(uint16_t); /* what the CPU reads then */
void io_dma_write(uint16_t, uint8_t); /* and what its write does: none of what it meant */
uint16_t io_dma_wram(uint16_t);       /* CGB: where an access to work RAM lands while DMA reads another bus */
int io_dma_byte(void);                /* the byte of OAM that DMA wrote in the last M-cycle, or -1 */

/* video.c */
enum {
  LCD_WIDTH = GB_LCD_WIDTH,
  LCD_HEIGHT = GB_LCD_HEIGHT,
  LCD_LINES = 154,   /* per frame, including the 10 of VBlank */
  LINE_CYCLES = 456, /* T-cycles per line */
  FRAME_CYCLES = LCD_LINES * LINE_CYCLES,
};

void video_reset(void);
void video_bios_init(void); /* the logo or palettes the boot ROM leaves */
void video_tick(uint32_t);
void video_stop(void); /* the screen with the clock stopped, by STOP */
uint8_t video_read(uint16_t);
void video_write(uint16_t, uint8_t);
uint32_t video_write_lead(uint16_t); /* T-cycles before its M-cycle's end that a write to this address lands */
int video_locked(uint16_t, int);     /* the PPU has VRAM or OAM, and the CPU's read (or if set, write) of it fails */
int video_dma_pending(void);         /* CGB: VRAM DMA has bytes to copy, with the CPU stopped */
void video_dma_begin(uint8_t);       /* it takes the bus, which has this on it */
void video_dma_byte(void);           /* and copies one */
const uint16_t *video_pixels(void);  /* LCD_WIDTH * LCD_HEIGHT: a DMG's shades, 0 (lightest) to 3, or a CGB's colors */
uint32_t video_frames(void);         /* frames finished so far; one more each time VBlank starts */

/* audio.c */
enum {
  AUDIO_START = 0xFF10,                        /* NR10, the first sound register */
  AUDIO_END = 0xFF3F,                          /* the last byte of wave RAM */
  AUDIO_SAMPLE_CYCLES = GB_AUDIO_TICKS,        /* T-cycles per output sample */
  AUDIO_RATE = CLOCK_HZ / AUDIO_SAMPLE_CYCLES, /* stereo samples a second, 47662 (and a half) */
};

void audio_reset(void);
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
  VRAM_BANKS = 2, /* CGB */
  WRAM_START = 0xC000,
  WRAM_SIZE = 0x2000,
  WRAM_BANK_SIZE = 0x1000, /* C000-CFFF is bank 0; D000-DFFF bank 1, or on a CGB, any of banks 1-7 */
  WRAM_BANKS = 8,
  ECHO_START = 0xE000, /* mirror of WRAM */
  OAM_START = 0xFE00,
  OAM_SIZE = 0xA0,
  HRAM_START = 0xFF80,
  HRAM_SIZE = 0x7F,
};

void mem_reset(int);         /* VRAM cleared, as the boot ROM does; and if set, all the rest */
void mem_wram_bank(uint8_t); /* CGB: the bank at D000-DFFF */
uint8_t *mem_wram(void);     /* all WRAM_BANKS of it */
uint8_t *mem_hram(void);

extern uint8_t oam[];
extern uint8_t vram[]; /* VRAM_BANKS of it */

uint8_t mem_read(uint16_t);
void mem_write(uint16_t, uint8_t);

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
extern uint8_t stopped;
extern uint8_t double_speed; /* CGB */
extern uint8_t cpu_locked;
extern uint32_t cpu_paused;   /* CGB: M-cycles left of the wait after a speed switch */
extern uint8_t cpu_switching; /* CGB: M-cycles until a speed switch that didn't stop the CPU takes effect */

int cpu_bios_init(void); /* everything as the boot ROM leaves it; 0 if the compiler laid F's bits out wrongly */
uint32_t step(void);     /* runs one instruction (or interrupt/halt cycle), returns T-cycles at the normal speed */

/* the CPU's memory accesses, each an M-cycle that the rest of the machine runs through; bus_idle() is one of the */
/* CPU's own, without an access. The PPU's share of an M-cycle is held back until the next access, or bus_sync(). */
uint8_t bus_read(uint16_t);
void bus_write(uint16_t, uint8_t);
void bus_idle(void);
void bus_sync(void); /* catches the PPU up, for looking at what it does (IF, say) without an access */

#endif /* _CORE_H_ */
