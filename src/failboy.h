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

#ifdef __cplusplus
extern "C" {
#endif

/* Failboy's core: a Game Boy, and nothing of the machine it runs on. A host gives it a cartridge's ROM and the */
/* buttons held down, runs it for as long as it likes, and takes the picture and the sound it makes. Files, windows, */
/* audio devices and real time are all the host's. */

/* Time is counted in ticks of the Game Boy's clock at its normal speed (T-cycles), whatever the host's clock is */
/* doing: gb_ticks() is what a host paces its video, audio and input against. The same ROM, saved state, clock and */
/* buttons at the same ticks always make the same run. */

/* A Game Boy belongs to one thread at a time; nothing here locks. Between runs, any of these functions can be */
/* called. So can any of them from inside a callback, which is the middle of a run, except the ones that run it */
/* further (gb_step(), gb_run(), gb_run_frame()) and gb_destroy(); gb_stop() ends the run from there. */

enum {
  GB_CLOCK_HZ = 4194304,
  GB_LCD_WIDTH = 160,
  GB_LCD_HEIGHT = 144,
  GB_FRAME_TICKS = 70224,                       /* 154 lines of 456: 59.73 frames a second */
  GB_AUDIO_TICKS = 88,                          /* per sample */
  GB_AUDIO_RATE = GB_CLOCK_HZ / GB_AUDIO_TICKS, /* stereo samples a second, 47662 (and a half) */
  GB_AUDIO_BUFFER = 4096,                       /* the most stereo samples the core holds on to, about 86 ms */
  GB_RTC_SIZE = 13,                             /* bytes of a cartridge clock's state */
};

/* The machine to be, down to the revision of its CPU, since they don't all behave alike. Most of them aren't */
/* supported yet (gb_create() says so); the ones that are: GB_DMG_ABC, and GB_CGB_A to GB_CGB_E, all taken to be the */
/* same so far. */
enum gb_revision {
  GB_REVISION_AUTO, /* what the cartridge's header asks for: GB_CGB_E for a color game, GB_DMG_ABC for any other */
  GB_DMG_0,         /* the original Game Boy; its first CPU, */
  GB_DMG_ABC,       /* and the usual ones */
  GB_MGB,           /* Game Boy Pocket */
  GB_SGB,           /* Super Game Boy */
  GB_SGB2,
  GB_CGB_0, /* Game Boy Color */
  GB_CGB_A,
  GB_CGB_B,
  GB_CGB_C,
  GB_CGB_D,
  GB_CGB_E,
  GB_AGB, /* a Game Boy Advance, being a Game Boy Color */
  GB_REVISIONS,
};

enum gb_error {
  GB_OK,
  GB_ERROR_ROM,       /* not a Game Boy ROM: they come in 32 kB * 2^n */
  GB_ERROR_CARTRIDGE, /* a type of cartridge (header byte 0x147) that isn't supported yet */
  GB_ERROR_REVISION,  /* a revision that isn't supported yet */
  GB_ERROR_MEMORY,    /* out of it */
  GB_ERROR_BUSY,      /* there's a Game Boy already; see gb_create() */
  GB_ERROR_BUILD,     /* the compiler that built the core laid the CPU's flags out the wrong way round */
};

/* buttons for gb_buttons(); a set bit means pressed */
enum {
  GB_BUTTON_RIGHT = 0x01,
  GB_BUTTON_LEFT = 0x02,
  GB_BUTTON_UP = 0x04,
  GB_BUTTON_DOWN = 0x08,
  GB_BUTTON_A = 0x10,
  GB_BUTTON_B = 0x20,
  GB_BUTTON_SELECT = 0x40,
  GB_BUTTON_START = 0x80,
};

/* What a pixel of gb_pixels() holds. The core leaves it at that: turning either into a color on the host's screen */
/* (a palette, the tint of the real LCD, its ghosting) is the host's business. */
enum gb_pixel_format {
  GB_PIXELS_SHADES, /* an original Game Boy: 0 (lightest) to 3 (darkest) */
  GB_PIXELS_COLORS, /* a Game Boy Color: 15 bits, 5 each of red (lowest), green and blue */
};

/* the memory behind the CPU's addresses, for gb_memory() */
enum gb_memory {
  GB_MEMORY_CART_RAM, /* the cartridge's RAM, if it has any: all of its banks, one after another */
  GB_MEMORY_VRAM,     /* 2 banks of 8 kB (the second is the Game Boy Color's) */
  GB_MEMORY_WRAM,     /* 8 banks of 4 kB (the original Game Boy has the first 2) */
  GB_MEMORY_OAM,      /* 160 bytes */
  GB_MEMORY_HRAM,     /* 127 bytes */
  GB_MEMORY_REGIONS,
};

struct gb;

/* the CPU, for gb_cpu() */
struct gb_cpu {
  uint16_t pc;
  uint16_t sp;
  uint16_t af;
  uint16_t bc;
  uint16_t de;
  uint16_t hl;
  uint8_t ime;     /* interrupts are enabled */
  uint8_t halted;  /* by HALT, until an interrupt is pending */
  uint8_t stopped; /* by STOP, until a button is pressed */
  uint8_t locked;  /* by an illegal opcode (the one before pc), for good */
};

/* What the core calls the host for, each with the Game Boy and the host's own `user`. Any of them can be left NULL. */
struct gb_host {
  void *user;

  /* A frame is finished (VBlank has begun): GB_LCD_WIDTH * GB_LCD_HEIGHT pixels, as gb_pixels() gives. With the LCD */
  /* off there are no frames, so a host that wants to keep drawing takes gb_pixels() itself. */
  void (*frame)(struct gb *gb, void *user, const uint16_t *pixels);

  /* The core's buffer of sound is full: GB_AUDIO_BUFFER stereo samples (left, right), which it then lets go of. */
  /* Without this, samples nobody has taken with gb_audio() by then are dropped. This one is called from the middle */
  /* of the sound being made, so gb_audio() there gives nothing new, and the sound registers read a little behind. */
  void (*audio)(struct gb *gb, void *user, const int16_t *samples, unsigned int count);

  /* The game sent a byte out of the link port, on its own clock. Nothing answers yet: it receives FF. */
  void (*serial)(struct gb *gb, void *user, uint8_t byte);

  /* The CPU has finished an instruction. */
  void (*trace)(struct gb *gb, void *user);

  /* The time, in seconds (Unix time), for a cartridge with a clock; without this, its clock stands still. */
  /* It's first called from gb_create(), before the host has the Game Boy. Provisional: the clock is due to count */
  /* the Game Boy's own ticks instead. */
  int64_t (*clock)(struct gb *gb, void *user);
};

struct gb_config {
  enum gb_revision revision;
  /* The cartridge. The core doesn't copy it, so it has to stay until gb_destroy(); and as it's read where it is, a */
  /* host can patch it there (a Game Genie code, say). */
  const uint8_t *rom;
  uint32_t rom_size;
  int doctor; /* be as Gameboy Doctor's logs expect: no PPU, LY reading 0x90, IF's unused bits 0, no serial interrupt */
  struct gb_host host;
};

/* Makes a Game Boy with the cartridge in, as its boot ROM leaves it, and stores it in *gb: no buttons held, and its */
/* RAM (the cartridge's too) all zeros, until the host loads a save. For now the core keeps its state in globals, so */
/* there can only be one, and only one per process: after the first, even once that's destroyed, this fails with */
/* GB_ERROR_BUSY. So there's no resetting one yet, either. */
enum gb_error gb_create(const struct gb_config *config, struct gb **gb);
void gb_destroy(struct gb *gb);

/* The callbacks it was made with, which can be changed: a tool that's handed a Game Boy can put its own in, and call */
/* the ones it found there. */
struct gb_host *gb_host(struct gb *gb);

enum gb_revision gb_revision(const struct gb *gb); /* the one it is, where the config said GB_REVISION_AUTO */
enum gb_pixel_format gb_pixel_format(const struct gb *gb);

/* Running. Each returns the ticks it ran for, which can be a few more than asked: an instruction is always finished. */

/* one instruction; or an interrupt's dispatch, or a moment of HALT or STOP */
uint32_t gb_step(struct gb *gb);

/* that many ticks, unless gb_stop() ends it sooner */
uint64_t gb_run(struct gb *gb, uint64_t ticks);

/* Until a frame is finished or gb_stop(); or, as there are no frames with the LCD off, GB_FRAME_TICKS from the call. */
uint32_t gb_run_frame(struct gb *gb);

/* from a callback: the run it's in the middle of ends after this instruction */
void gb_stop(struct gb *gb);

/* Ticks run since it was made. In a callback, that's up to the start of the instruction under way. */
uint64_t gb_ticks(const struct gb *gb);

/* Frames finished since it was made; in the frame callback, that one too. */
uint32_t gb_frames(const struct gb *gb);

/* The GB_BUTTON_s held down, from this tick until the next call. A press is what requests the joypad interrupt and */
/* ends STOP. */
void gb_buttons(struct gb *gb, uint8_t pressed);

/* The picture as it stands, GB_LCD_WIDTH * GB_LCD_HEIGHT pixels: the last frame, with whatever of the next has been */
/* drawn over it, or blank with the LCD off. It's always the same buffer, there until gb_destroy(). */
const uint16_t *gb_pixels(const struct gb *gb);

/* The sound made since the last call (or the last audio callback), right up to this tick: stereo samples (left, */
/* right), and how many, at most GB_AUDIO_BUFFER. They're the host's until it next runs the Game Boy. There's one */
/* every GB_AUDIO_TICKS, except while STOP has the clock stopped, so a recording keeps time by counting them. */
const int16_t *gb_audio(struct gb *gb, unsigned int *count);

void gb_cpu(const struct gb *gb, struct gb_cpu *cpu);

/* Memory, as the CPU sees it now (with the banks it has selected), but taking no time, and never blocked as the */
/* CPU's own access can be (by OAM DMA). A write does all that the CPU's would: below 8000 it goes to the cartridge's */
/* mapper, not the ROM, and a register does what it does. */
uint8_t gb_read(struct gb *gb, uint16_t address);
void gb_write(struct gb *gb, uint16_t address, uint8_t value);

/* The memory itself, all of its banks whatever the CPU has selected (and whichever of them the revision has), and */
/* its size; NULL if there's none. It stays where it is until gb_destroy(). Writing to it changes just the bytes. */
uint8_t *gb_memory(struct gb *gb, enum gb_memory region, uint32_t *size);

/* What a cartridge keeps with the power off, for the host to keep in a file: the RAM that has a battery, and the */
/* state of a clock. */
uint32_t gb_save_size(const struct gb *gb); /* 0 unless the cartridge's RAM has a battery */
void gb_save_read(const struct gb *gb, uint8_t *data, uint32_t size);
void gb_save_write(struct gb *gb, const uint8_t *data, uint32_t size);
int gb_has_rtc(const struct gb *gb);
void gb_rtc_read(const struct gb *gb, uint8_t *data, uint32_t size);  /* GB_RTC_SIZE bytes */
void gb_rtc_write(struct gb *gb, const uint8_t *data, uint32_t size); /* a saved clock catches up on the time since */

#ifdef __cplusplus
}
#endif

#endif /* _FAILBOY_H_ */
