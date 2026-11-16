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

/* The core as its hosts see it (failboy.h), and the way back out to them: the host's callbacks. */

#include <stddef.h>

#include "core.h"
#include "rtc.h"

/* The one Game Boy. The machine itself is in each module's globals, so this is only what the host gave it and what */
/* its runs keep count of. */
struct gb {
  struct gb_host host;
  uint64_t ticks;
  int stop;   /* gb_stop() was called; the run under way ends */
  int exists; /* made, and not yet destroyed */
};

static struct gb gb_only;

int doctor = 0;
int revision = GB_HARDWARE_DMG_B;
uint8_t cgb = 0;
uint8_t cgb_d = 0;
uint8_t cgb_mode = 0;

/* the revision the core is, for each console a host can ask for */
static const int hardware[GB_REVISIONS] = {
    [GB_DMG_AUTO] = GB_HARDWARE_DMG_B,
    [GB_CGB_AUTO] = GB_HARDWARE_CGB_D,
    [GB_CGB_C] = GB_HARDWARE_CGB_C,
    [GB_CGB_D] = GB_HARDWARE_CGB_D,
};

/* **************************************** */
/* The host's callbacks */

void host_frame(void) {
  if (gb_only.host.frame != NULL) {
    gb_only.host.frame(&gb_only, gb_only.host.user, video_pixels());
  }
}

int host_audio(const int16_t *samples, unsigned int count) {
  if (gb_only.host.audio == NULL) {
    return 0;
  }
  gb_only.host.audio(&gb_only, gb_only.host.user, samples, count);
  return 1;
}

void host_serial(uint8_t byte) {
  if (gb_only.host.serial != NULL) {
    gb_only.host.serial(&gb_only, gb_only.host.user, byte);
  }
}

void host_trace(void) {
  if (gb_only.host.trace != NULL) {
    gb_only.host.trace(&gb_only, gb_only.host.user);
  }
}

int64_t host_clock(void) {
  if (gb_only.host.clock == NULL) {
    return 0;
  }
  return gb_only.host.clock(&gb_only, gb_only.host.user);
}

/* **************************************** */
/* Making one */

/* The power comes on: every module back to how it starts, then as the boot ROM leaves it. Returns 0 if the CPU's */
/* flags are laid out wrongly (see cpu_bios_init()). */
static int power_on(int clear_ram) {
  mem_reset(clear_ram);
  cart_reset(clear_ram);
  io_reset();
  video_reset();
  audio_reset();
  gb_only.ticks = 0;
  gb_only.stop = 0;
  return cpu_bios_init();
}

enum gb_error gb_create(const struct gb_config *config, struct gb **gb) {
  *gb = NULL;
  if (gb_only.exists) {
    return GB_ERROR_BUSY;
  }
  if ((unsigned int)config->revision >= GB_REVISIONS) {
    return GB_ERROR_REVISION;
  }
  /* first, as a cartridge's clock starts from the host's */
  gb_only.host = config->host;
  enum gb_error error = cart_load(config->rom, config->rom_size);
  if (error != GB_OK) {
    return error;
  }

  enum gb_revision console = config->revision;
  if (console == GB_AUTO) {
    console = GB_DMG_AUTO;
    if (cart_cgb()) {
      console = GB_CGB_AUTO;
    }
  }
  revision = hardware[console];
  cgb = (revision & GB_CONSOLE) == GB_CONSOLE_CGB;
  cgb_d = revision == GB_HARDWARE_CGB_D;
  /* a Game Boy Color runs an original Game Boy's game in its compatibility mode */
  cgb_mode = cgb && cart_cgb();
  doctor = config->doctor != 0;

  if (!power_on(1)) {
    cart_free();
    return GB_ERROR_BUILD;
  }
  gb_only.exists = 1;
  *gb = &gb_only;
  return GB_OK;
}

void gb_destroy(struct gb *gb) {
  if (gb == NULL || !gb->exists) {
    return;
  }
  gb->exists = 0;
  cart_free();
}

void gb_reset(struct gb *gb, int clear_ram) {
  (void)gb;
  power_on(clear_ram);
}

struct gb_host *gb_host(struct gb *gb) { return &gb->host; }

enum gb_hardware gb_hardware(const struct gb *gb) {
  (void)gb;
  return (enum gb_hardware)revision;
}

enum gb_pixel_format gb_pixel_format(const struct gb *gb) {
  (void)gb;
  if (cgb) {
    return GB_PIXELS_COLORS;
  }
  return GB_PIXELS_SHADES;
}

/* **************************************** */
/* Running */

uint32_t gb_step(struct gb *gb) {
  uint32_t ticks = step();
  gb->ticks += ticks;
  if (cart_has_rtc()) {
    rtc_advance(ticks);
  }
  return ticks;
}

uint64_t gb_run(struct gb *gb, uint64_t ticks) {
  uint64_t ran = 0;
  gb->stop = 0;
  while (ran < ticks && !gb->stop) {
    ran += gb_step(gb);
  }
  return ran;
}

uint32_t gb_run_frame(struct gb *gb) {
  uint32_t frame = video_frames();
  uint32_t ran = 0;
  gb->stop = 0;
  while (video_frames() == frame && ran < FRAME_CYCLES && !gb->stop) {
    ran += gb_step(gb);
  }
  return ran;
}

void gb_stop(struct gb *gb) { gb->stop = 1; }

uint64_t gb_ticks(const struct gb *gb) { return gb->ticks; }

uint32_t gb_frames(const struct gb *gb) {
  (void)gb;
  return video_frames();
}

/* **************************************** */
/* Buttons, picture, sound */

void gb_buttons(struct gb *gb, uint8_t pressed) {
  (void)gb;
  io_joypad(pressed);
}

const uint16_t *gb_pixels(const struct gb *gb) {
  (void)gb;
  return video_pixels();
}

const int16_t *gb_audio(struct gb *gb, unsigned int *count) {
  (void)gb;
  return audio_samples(count);
}

/* **************************************** */
/* The CPU and memory */

void gb_cpu(const struct gb *gb, struct gb_cpu *cpu) {
  (void)gb;
  cpu->pc = r.PC;
  cpu->sp = r.SP;
  cpu->af = r.AF;
  cpu->bc = r.BC;
  cpu->de = r.DE;
  cpu->hl = r.HL;
  cpu->ime = ime;
  cpu->halted = halted;
  cpu->stopped = stopped;
  cpu->locked = cpu_locked;
}

uint8_t gb_read(struct gb *gb, uint16_t address) {
  (void)gb;
  return mem_read(address);
}

void gb_write(struct gb *gb, uint16_t address, uint8_t value) {
  (void)gb;
  mem_write(address, value);
}

uint8_t *gb_memory(struct gb *gb, enum gb_memory region, uint32_t *size) {
  (void)gb;
  switch (region) {
    case GB_MEMORY_CART_RAM:
      return cart_ram(size);
    case GB_MEMORY_VRAM:
      *size = VRAM_BANKS * VRAM_SIZE;
      return vram;
    case GB_MEMORY_WRAM:
      *size = WRAM_BANKS * WRAM_BANK_SIZE;
      return mem_wram();
    case GB_MEMORY_OAM:
      *size = OAM_SIZE;
      return oam;
    case GB_MEMORY_HRAM:
      *size = HRAM_SIZE;
      return mem_hram();
    default:
      *size = 0;
      return NULL;
  }
}

/* **************************************** */
/* What the cartridge keeps */

uint32_t gb_save_size(const struct gb *gb) {
  (void)gb;
  return cart_battery_ram_size();
}

void gb_save_read(const struct gb *gb, uint8_t *data, uint32_t size) {
  (void)gb;
  cart_read_battery_ram(data, size);
}

void gb_save_write(struct gb *gb, const uint8_t *data, uint32_t size) {
  (void)gb;
  cart_write_battery_ram(data, size);
}

int gb_has_rtc(const struct gb *gb) {
  (void)gb;
  return cart_has_rtc();
}

void gb_rtc_read(const struct gb *gb, uint8_t *data, uint32_t size) {
  (void)gb;
  cart_read_rtc(data, size);
}

void gb_rtc_write(struct gb *gb, const uint8_t *data, uint32_t size) {
  (void)gb;
  cart_write_rtc(data, size);
}
