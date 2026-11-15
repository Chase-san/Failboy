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

/* What failboy.h promises its hosts: making, resetting and destroying a Game Boy, running and stopping it, and that */
/* the same run always comes out the same. Any ROM files given as arguments get that last check too. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tests/test.h"

enum {
  RUN_FRAMES = 30,
  STOP_AT_FRAME = 5,
  ROM_FRAMES = 120,       /* for the ROM files */
  INSTRUCTION_TICKS = 24, /* the longest instruction */
  IO_START = 0xFF00,
  IO_END = 0xFF80,
  IE = 0xFFFF,
  WRAM_START = 0xC000,
  WRAM_BANK_1 = 0xD000,
  ECHO_START = 0xE000,
  HRAM_START = 0xFF80,
  VRAM_START = 0x8000,
  CART_RAM_START = 0xA000,
  RAM_ENABLE = 0x0A,
  TIMER_VECTOR = 0x0050,
  OP_RETI = 0xD9,
  OP_ILLEGAL = 0xD3,
  CART_MBC1_RAM = 0x02,
  CART_MBC1_RAM_BATT = 0x03,
  CART_UNKNOWN = 0xFC,
  RAM_8K = 0x02, /* the header's code for it */
  RAM_8K_SIZE = 0x2000,
  BAD_ROM_SIZE = 1000,
  REVISION_OUT_OF_RANGE = 99,
};

/* Keeps the machine busy: the timer and its interrupt, a sound channel, OAM DMA (from VRAM, so it isn't in the */
/* program's way), and a counter in work RAM that goes into the palette and the channel's frequency. */
static const uint8_t busy[] = {
    0x3E, 0x04, 0xE0, 0xFF,  // ld a,04 / ldh (FF),a: the timer's interrupt
    0x3E, 0x05, 0xE0, 0x07,  // ld a,05 / ldh (07),a: the timer on, at its fastest
    0x3E, 0xF3, 0xE0, 0x12,  // ld a,F3 / ldh (12),a: channel 1's envelope
    0x3E, 0x87, 0xE0, 0x14,  // ld a,87 / ldh (14),a: trigger it
    0x3E, 0x80, 0xE0, 0x46,  // ld a,80 / ldh (46),a: OAM DMA
    0xFB,                    // ei
    0x21, 0x00, 0xC0,        // ld hl,C000 (the loop)
    0x34,                    // inc (hl)
    0x7E,                    // ld a,(hl)
    0xE0, 0x47,              // ldh (47),a: BGP
    0xE0, 0x13,              // ldh (13),a: channel 1's frequency
    0x18, 0xF5,              // jr to the loop
};

/* the buttons held on each frame of a run, over and over */
static const uint8_t presses[] = {
    0, GB_BUTTON_A, GB_BUTTON_A | GB_BUTTON_RIGHT, 0, GB_BUTTON_START, GB_BUTTON_DOWN, 0, GB_BUTTON_B,
};

/* where a run ended up */
struct outcome {
  struct gb_cpu cpu;
  uint64_t ticks;
  uint32_t frames;
  uint32_t pixels;
  uint32_t sound;
  uint32_t memory;
  uint32_t registers;
};

/* what the callbacks have seen */
static unsigned int frames_seen = 0;
static unsigned int samples_seen = 0;
static unsigned int audio_calls = 0;
static unsigned int traces_seen = 0;
static uint32_t sound_hash = TEST_HASH_START;
static uint64_t stop_tick = 0;

static void on_frame(struct gb *gb, void *user, const uint16_t *pixels) {
  (void)user;
  CHECK(pixels == gb_pixels(gb));
  ++frames_seen;
  CHECK(gb_frames(gb) == frames_seen);
  if (frames_seen == STOP_AT_FRAME) {
    gb_stop(gb);
    stop_tick = gb_ticks(gb);
  }
}

static void on_audio(struct gb *gb, void *user, const int16_t *samples, unsigned int count) {
  (void)gb;
  (void)user;
  ++audio_calls;
  samples_seen += count;
  sound_hash = test_hash(samples, count * 2 * sizeof(samples[0]), sound_hash);
}

static void on_trace(struct gb *gb, void *user) {
  (void)gb;
  (void)user;
  ++traces_seen;
}

static uint32_t hash_region(struct gb *gb, enum gb_memory region, uint32_t hash) {
  uint32_t size = 0;
  const uint8_t *memory = gb_memory(gb, region, &size);
  if (memory == NULL) {
    return hash;
  }
  return test_hash(memory, size, hash);
}

/* Runs some frames with buttons going up and down, and takes down everything a host can see of where that got to. */
static void run(struct gb *gb, int frames, struct outcome *outcome) {
  sound_hash = TEST_HASH_START;
  for (int i = 0; i < frames; ++i) {
    gb_buttons(gb, presses[i % (int)sizeof(presses)]);
    gb_run_frame(gb);
  }
  memset(outcome, 0, sizeof(*outcome));
  gb_cpu(gb, &outcome->cpu);
  outcome->ticks = gb_ticks(gb);
  outcome->frames = gb_frames(gb);
  outcome->pixels = test_hash(gb_pixels(gb), GB_LCD_WIDTH * GB_LCD_HEIGHT * sizeof(uint16_t), TEST_HASH_START);
  unsigned int count = 0;
  const int16_t *samples = gb_audio(gb, &count);
  outcome->sound = test_hash(samples, count * 2 * sizeof(samples[0]), sound_hash);
  outcome->memory = TEST_HASH_START;
  for (int region = 0; region < GB_MEMORY_REGIONS; ++region) {
    outcome->memory = hash_region(gb, (enum gb_memory)region, outcome->memory);
  }
  outcome->registers = TEST_HASH_START;
  for (unsigned int address = IO_START; address < IO_END; ++address) {
    uint8_t value = gb_read(gb, (uint16_t)address);
    outcome->registers = test_hash(&value, 1, outcome->registers);
  }
  uint8_t ie = gb_read(gb, IE);
  outcome->registers = test_hash(&ie, 1, outcome->registers);
  gb_buttons(gb, 0);
}

static int same(const struct outcome *a, const struct outcome *b) { return memcmp(a, b, sizeof(*a)) == 0; }

/* gb_create() turns down what it can't be, and makes only one at a time */
static void test_create(void) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, 0, 0);
  struct gb_config config = {0};
  struct gb *gb = NULL;
  struct gb *second = NULL;
  config.rom = rom;
  config.rom_size = BAD_ROM_SIZE;
  CHECK(gb_create(&config, &gb) == GB_ERROR_ROM && gb == NULL);
  config.rom = NULL;
  config.rom_size = TEST_ROM_SIZE;
  CHECK(gb_create(&config, &gb) == GB_ERROR_ROM);
  config.rom = rom;
  config.revision = GB_SGB_AUTO;
  CHECK(gb_create(&config, &gb) == GB_ERROR_REVISION && gb == NULL);
  config.revision = (enum gb_revision)REVISION_OUT_OF_RANGE;
  CHECK(gb_create(&config, &gb) == GB_ERROR_REVISION);
  config.revision = GB_AUTO;
  uint8_t *unknown = test_rom(TEST_ROM_SIZE, CART_UNKNOWN, 0);
  config.rom = unknown;
  CHECK(gb_create(&config, &gb) == GB_ERROR_CARTRIDGE);
  config.rom = rom;

  /* none of those left anything behind */
  CHECK(gb_create(&config, &gb) == GB_OK && gb != NULL);
  CHECK(gb_pixel_format(gb) == GB_PIXELS_SHADES);
  CHECK(gb_ticks(gb) == 0 && gb_frames(gb) == 0);
  CHECK(gb_create(&config, &second) == GB_ERROR_BUSY && second == NULL);
  gb_destroy(gb);

  /* the header picks the console, unless the host does */
  rom[TEST_HEADER_CGB] = TEST_HEADER_CGB_GAME;
  CHECK(gb_create(&config, &gb) == GB_OK);
  CHECK(gb_pixel_format(gb) == GB_PIXELS_COLORS);
  gb_destroy(gb);
  config.revision = GB_DMG_AUTO;
  CHECK(gb_create(&config, &gb) == GB_OK);
  CHECK(gb_pixel_format(gb) == GB_PIXELS_SHADES);
  gb_destroy(gb);
  rom[TEST_HEADER_CGB] = 0;
  config.revision = GB_CGB_AUTO;
  CHECK(gb_create(&config, &gb) == GB_OK);
  CHECK(gb_pixel_format(gb) == GB_PIXELS_COLORS);
  gb_destroy(gb);
  gb_destroy(gb); /* and again does no harm */
  free(unknown);
  free(rom);
}

/* running for so long, stopping from a callback, and every sample accounted for */
static void test_running(void) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, 0, 0);
  test_program(rom, busy, sizeof(busy));
  rom[TIMER_VECTOR] = OP_RETI;
  struct gb_host host = {0};
  host.frame = on_frame;
  host.audio = on_audio;
  struct gb *gb = test_boot(GB_DMG_AUTO, rom, TEST_ROM_SIZE, &host);
  frames_seen = samples_seen = audio_calls = 0;

  uint64_t ran = gb_run(gb, (uint64_t)GB_FRAME_TICKS * RUN_FRAMES);
  CHECK(frames_seen == STOP_AT_FRAME && gb_frames(gb) == STOP_AT_FRAME);
  CHECK(ran == gb_ticks(gb));
  CHECK(ran - stop_tick <= INSTRUCTION_TICKS);
  CHECK(ran < (uint64_t)GB_FRAME_TICKS * (STOP_AT_FRAME + 1));

  uint64_t before = gb_ticks(gb);
  ran = gb_run(gb, GB_CLOCK_HZ);
  CHECK(ran >= GB_CLOCK_HZ && ran < GB_CLOCK_HZ + INSTRUCTION_TICKS);
  CHECK(gb_ticks(gb) == before + ran);
  uint32_t frame = gb_run_frame(gb);
  CHECK(frame > 0 && frame <= GB_FRAME_TICKS + INSTRUCTION_TICKS);

  unsigned int left = 0;
  gb_audio(gb, &left);
  CHECK(audio_calls > 0 && samples_seen == audio_calls * GB_AUDIO_BUFFER);
  CHECK(samples_seen + left == gb_ticks(gb) / GB_AUDIO_TICKS);
  gb_audio(gb, &left);
  CHECK(left == 0);

  /* a tool can put its own callbacks in afterwards */
  traces_seen = 0;
  gb_host(gb)->trace = on_trace;
  CHECK(gb_host(gb)->frame == on_frame);
  gb_step(gb);
  gb_step(gb);
  CHECK(traces_seen > 0 && traces_seen <= 2);
  gb_destroy(gb);
  free(rom);
}

/* the CPU's view of memory, and the memory itself, are the same bytes */
static void test_memory(void) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, 0, 0);
  struct gb *gb = test_boot(GB_DMG_AUTO, rom, TEST_ROM_SIZE, NULL);
  uint32_t size = 0;
  uint8_t *wram = gb_memory(gb, GB_MEMORY_WRAM, &size);
  CHECK(wram != NULL && size == 0x8000);
  gb_write(gb, WRAM_START + 0x123, 0x5A);
  CHECK(wram[0x123] == 0x5A);
  CHECK(gb_read(gb, WRAM_START + 0x123) == 0x5A && gb_read(gb, ECHO_START + 0x123) == 0x5A);
  wram[0x1234] = 0xA5;
  CHECK(gb_read(gb, WRAM_BANK_1 + 0x234) == 0xA5);
  uint8_t *hram = gb_memory(gb, GB_MEMORY_HRAM, &size);
  CHECK(hram != NULL && size == 127);
  gb_write(gb, HRAM_START + 0x10, 0x77);
  CHECK(hram[0x10] == 0x77);
  uint8_t *vram = gb_memory(gb, GB_MEMORY_VRAM, &size);
  CHECK(vram != NULL && size == 0x4000);
  CHECK(vram[0x10] == gb_read(gb, VRAM_START + 0x10));
  CHECK(gb_memory(gb, GB_MEMORY_OAM, &size) != NULL && size == 160);
  CHECK(gb_memory(gb, GB_MEMORY_CART_RAM, &size) == NULL && size == 0);
  CHECK(gb_memory(gb, GB_MEMORY_REGIONS, &size) == NULL && size == 0);
  /* the ROM is read where the host has it, so a patch there shows */
  rom[TEST_PROGRAM] = 0x42;
  CHECK(gb_read(gb, TEST_PROGRAM) == 0x42);
  gb_destroy(gb);
  free(rom);
}

/* an illegal opcode hangs the CPU, and time goes on without it */
static void test_locked(void) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, 0, 0);
  rom[TEST_ENTRY] = OP_ILLEGAL;
  struct gb *gb = test_boot(GB_DMG_AUTO, rom, TEST_ROM_SIZE, NULL);
  struct gb_cpu cpu;
  gb_step(gb);
  gb_cpu(gb, &cpu);
  CHECK(cpu.locked && cpu.pc == TEST_ENTRY + 1);
  uint64_t ran = gb_run(gb, (uint64_t)GB_FRAME_TICKS * 2);
  CHECK(ran >= (uint64_t)GB_FRAME_TICKS * 2);
  CHECK(gb_frames(gb) >= 1);
  gb_cpu(gb, &cpu);
  CHECK(cpu.locked && cpu.pc == TEST_ENTRY + 1);
  gb_reset(gb);
  gb_cpu(gb, &cpu);
  CHECK(!cpu.locked && cpu.pc == TEST_ENTRY);
  gb_destroy(gb);
  free(rom);
}

/* A run comes out the same from a new Game Boy, a reset one, and one made after others have been and gone. */
static void test_same_again(const uint8_t *rom, uint32_t size, int frames) {
  struct gb_host host = {0};
  host.audio = on_audio;
  struct outcome first;
  struct outcome again;

  struct gb *gb = test_boot(GB_AUTO, rom, size, &host);
  run(gb, frames, &first);
  CHECK(first.ticks > 0);
  gb_reset(gb);
  CHECK(gb_ticks(gb) == 0 && gb_frames(gb) == 0);
  run(gb, frames, &again);
  CHECK(same(&first, &again));
  gb_destroy(gb);

  /* as the other console, which leaves different things lying about */
  enum gb_revision other = GB_CGB_AUTO;
  if (rom[TEST_HEADER_CGB] & TEST_HEADER_CGB_GAME) {
    other = GB_DMG_AUTO;
  }
  gb = test_boot(other, rom, size, &host);
  run(gb, frames, &again);
  gb_destroy(gb);

  gb = test_boot(GB_AUTO, rom, size, &host);
  run(gb, frames, &again);
  CHECK(same(&first, &again));
  gb_destroy(gb);
}

static void test_busy_same_again(void) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, 0, 0);
  test_program(rom, busy, sizeof(busy));
  rom[TIMER_VECTOR] = OP_RETI;
  test_same_again(rom, TEST_ROM_SIZE, RUN_FRAMES);

  /* and the program did get somewhere */
  struct gb *gb = test_boot(GB_AUTO, rom, TEST_ROM_SIZE, NULL);
  struct outcome outcome;
  run(gb, RUN_FRAMES, &outcome);
  CHECK(outcome.frames == RUN_FRAMES);
  CHECK(outcome.cpu.pc >= TEST_PROGRAM && outcome.cpu.pc < TEST_PROGRAM + sizeof(busy));
  CHECK(gb_read(gb, WRAM_START) != 0);
  gb_destroy(gb);

  rom[TEST_HEADER_CGB] = TEST_HEADER_CGB_GAME;
  test_same_again(rom, TEST_ROM_SIZE, RUN_FRAMES);
  free(rom);
}

/* a reset keeps the cartridge's RAM only if it has a battery */
static void test_reset_ram(uint8_t type, int kept) {
  uint8_t *rom = test_rom(TEST_ROM_SIZE, type, RAM_8K);
  struct gb *gb = test_boot(GB_DMG_AUTO, rom, TEST_ROM_SIZE, NULL);
  uint32_t size = 0;
  uint8_t *ram = gb_memory(gb, GB_MEMORY_CART_RAM, &size);
  CHECK(ram != NULL && size == RAM_8K_SIZE);
  CHECK(gb_read(gb, CART_RAM_START) == 0xFF); /* switched off */
  gb_write(gb, 0x0000, RAM_ENABLE);
  gb_write(gb, CART_RAM_START, 0x99);
  CHECK(ram[0] == 0x99 && gb_read(gb, CART_RAM_START) == 0x99);
  if (kept) {
    CHECK(gb_save_size(gb) == RAM_8K_SIZE);
  } else {
    CHECK(gb_save_size(gb) == 0);
  }
  gb_write(gb, WRAM_START, 0x11);
  gb_reset(gb);
  CHECK(gb_read(gb, WRAM_START) == 0);
  CHECK(gb_read(gb, CART_RAM_START) == 0xFF); /* switched off again */
  if (kept) {
    CHECK(ram[0] == 0x99);
  } else {
    CHECK(ram[0] == 0);
  }
  gb_destroy(gb);
  free(rom);
}

static uint8_t *load(const char *filename, uint32_t *size) {
  FILE *f = fopen(filename, "rb");
  if (f == NULL) {
    return NULL;
  }
  uint8_t *data = NULL;
  long length = 0;
  if (fseek(f, 0, SEEK_END) == 0 && (length = ftell(f)) > 0 && fseek(f, 0, SEEK_SET) == 0) {
    data = malloc(length);
  }
  if (data != NULL && fread(data, length, 1, f) != 1) {
    free(data);
    data = NULL;
  }
  fclose(f);
  *size = (uint32_t)length;
  return data;
}

int main(int argc, char *argv[]) {
  test_create();
  test_running();
  test_memory();
  test_locked();
  test_busy_same_again();
  test_reset_ram(CART_MBC1_RAM, 0);
  test_reset_ram(CART_MBC1_RAM_BATT, 1);
  for (int i = 1; i < argc; ++i) {
    uint32_t size = 0;
    uint8_t *rom = load(argv[i], &size);
    CHECK(rom != NULL);
    if (rom != NULL) {
      test_same_again(rom, size, ROM_FRAMES);
      free(rom);
    }
  }
  return test_result("api");
}
