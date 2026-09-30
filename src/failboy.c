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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"

#ifdef FAILBOY_SDL
#include "sdl/frontend.h"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

enum {
  DEFAULT_SECONDS = 120, /* headless only; the window runs until it's closed */
  TRACE_BUFFER_SIZE = 1 << 20,
};

/* exit status */
enum {
  EXIT_PASSED = 0,
  EXIT_FAILED = 1,
  EXIT_ERROR = 2,
  EXIT_NO_RESULT = 3,
};

/* --dump writes an 8-bit greyscale PGM, with the DMG's four shades, or on a CGB, a PPM */
static const uint8_t dump_gray[4] = {0xFF, 0xAA, 0x55, 0x00};

/* a CGB color's 5-bit components (red lowest), widened to 8 for the PPM */
enum {
  COMPONENTS = 3,
  COMPONENT_BITS = 5,
  COMPONENT_MASK = 0x1F,
  WIDEN_SHIFT = 8 - COMPONENT_BITS, /* and its top bits repeated below */
};

static const char SAVE_EXTENSION[] = ".sav";
static const char RTC_EXTENSION[] = ".rtc";

int doctor = 0;
int model = MODEL_DMG;

/* Release builds on Windows are windowed programs with no console of their own. When one is started from a */
/* terminal, borrow that terminal for anything not already redirected, so usage, errors and --headless output show. */
static void attach_console(void) {
#ifdef _WIN32
  int out = GetStdHandle(STD_OUTPUT_HANDLE) == NULL;
  int err = GetStdHandle(STD_ERROR_HANDLE) == NULL;
  if ((out || err) && AttachConsole(ATTACH_PARENT_PROCESS)) {
    if (out) {
      freopen("CONOUT$", "w", stdout);
    }
    if (err) {
      freopen("CONOUT$", "w", stderr);
    }
  }
#endif
}

static int usage(const char *name) {
  fprintf(stderr, "usage: %s [options] rom.gb\n", name);
#ifdef FAILBOY_SDL
  fprintf(stderr, "  --rslcd      start with the really shitty LCD (L toggles it)\n");
  fprintf(stderr, "  --headless   run without a window (implied by the options below)\n");
#endif
  fprintf(stderr, "  --gb         run as an original Game Boy, even a color game\n");
  fprintf(stderr, "  --gbc        run as a Game Boy Color, even an original Game Boy game\n");
  fprintf(stderr, "               (without either, as whatever the game's header says it's for)\n");
  fprintf(stderr, "  --doctor     print a Gameboy Doctor trace to stdout (serial output goes to stderr)\n");
  fprintf(stderr, "  --frames N   stop after N frames\n");
  fprintf(stderr, "  --dump FILE  save the last frame as a PGM image (a PPM for a Game Boy Color)\n");
  fprintf(stderr, "  --seconds N  stop after N emulated seconds (headless default %d)\n", DEFAULT_SECONDS);
  fprintf(stderr, "  --save FILE  keep the cartridge's battery save in FILE (default: the ROM's name, ending %s);\n",
          SAVE_EXTENSION);
  fprintf(stderr, "               a cartridge clock is kept beside it, ending %s\n", RTC_EXTENSION);
  fprintf(stderr, "exit status: %d passed, %d failed, %d error, %d no result\n", EXIT_PASSED, EXIT_FAILED, EXIT_ERROR,
          EXIT_NO_RESULT);
  return EXIT_ERROR;
}

static int dump_frame(const char *filename) {
  FILE *f = fopen(filename, "wb");
  if (f == NULL) {
    fprintf(stderr, "failboy: can't write '%s'\n", filename);
    return 0;
  }
  if (model == MODEL_DMG) {
    const uint8_t *frame = video_framebuffer();
    fprintf(f, "P5\n%d %d\n255\n", LCD_WIDTH, LCD_HEIGHT);
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; ++i) {
      fputc(dump_gray[frame[i]], f);
    }
  } else {
    const uint16_t *frame = video_colors();
    fprintf(f, "P6\n%d %d\n255\n", LCD_WIDTH, LCD_HEIGHT);
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; ++i) {
      for (int component = 0; component < COMPONENTS; ++component) {
        uint8_t c = (frame[i] >> (component * COMPONENT_BITS)) & COMPONENT_MASK;
        fputc(c << WIDEN_SHIFT | c >> (COMPONENT_BITS - WIDEN_SHIFT), f);
      }
    }
  }
  if (fclose(f) != 0) {
    fprintf(stderr, "failboy: can't write '%s'\n", filename);
    return 0;
  }
  return 1;
}

static void run_headless(uint64_t limit, unsigned long frames) {
  uint64_t elapsed = 0; /* T-cycles at the normal speed */
  while (!cpu_locked && elapsed < limit) {
    uint16_t pc = r.PC;
    elapsed += step();
    if (frames) {
      if (video_frames() >= frames) {
        break;
      }
    } else if (r.PC == pc && !halted && !ime) {
      /* Jumping to itself with interrupts off: nothing can ever get it out (test ROMs end this way). */
      break;
    }
    if (doctor && ferror(stdout)) {
      break;
    }
  }
  fflush(stdout);
}

/* path with a different extension: rom.gb -> rom.sav, rom.sav -> rom.rtc */
static char *with_extension(const char *path, const char *extension) {
  const char *name = path;
  for (const char *p = path; *p != '\0'; ++p) {
    if (*p == '/' || *p == '\\') {
      name = p + 1;
    }
  }
  size_t length = strlen(path);
  const char *dot = strrchr(name, '.');
  if (dot != NULL && dot != name) {
    length = dot - path;
  }
  size_t extension_size = strlen(extension) + 1;
  char *result = malloc(length + extension_size);
  if (result != NULL) {
    memcpy(result, path, length);
    memcpy(result + length, extension, extension_size);
  }
  return result;
}

/* Loads saved state (battery RAM or the clock) into the cartridge, if there is any. Returns 0 if there's a save that */
/* can't be read, which then mustn't be saved over. */
static int load_state(const char *path, void (*restore)(const uint8_t *, unsigned int)) {
  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    if (errno == ENOENT) {
      /* no save yet */
      return 1;
    }
    fprintf(stderr, "failboy: can't open the save file '%s', so it won't be saved over\n", path);
    return 0;
  }
  int empty = fgetc(f) == EOF;
  fclose(f);
  if (empty) {
    return 1;
  }
  unsigned int size = 0;
  uint8_t *data = file_load(path, &size);
  if (data == NULL) {
    fprintf(stderr, "failboy: can't read the save file '%s', so it won't be saved over\n", path);
    return 0;
  }
  restore(data, size);
  free(data);
  return 1;
}

static int save_state(const char *path, unsigned int size, void (*collect)(uint8_t *, unsigned int)) {
  uint8_t *data = malloc(size);
  int ok = data != NULL;
  if (ok) {
    collect(data, size);
    ok = file_save(path, data, size);
  }
  if (!ok) {
    fprintf(stderr, "failboy: can't write the save file '%s'\n", path);
  }
  free(data);
  return ok;
}

static int run_window(uint64_t limit, int rslcd) {
#ifdef FAILBOY_SDL
  return frontend_run(limit, rslcd);
#else
  (void)limit;
  (void)rslcd;
  return 0;
#endif
}

int main(int argc, char *argv[]) {
  const char *filename = NULL;
  const char *dump = NULL;
  const char *save_option = NULL;
  unsigned long seconds = 0;
  unsigned long frames = 0;
  int headless = 1;
  int rslcd = 0;
  int gb = 0;
  int gbc = 0;
#ifdef FAILBOY_SDL
  headless = 0;
#endif

  attach_console();
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--headless") == 0) {
      headless = 1;
#ifdef FAILBOY_SDL
    } else if (strcmp(argv[i], "--rslcd") == 0) {
      rslcd = 1;
#endif
    } else if (strcmp(argv[i], "--gb") == 0) {
      gb = 1;
      gbc = 0;
    } else if (strcmp(argv[i], "--gbc") == 0) {
      gbc = 1;
      gb = 0;
    } else if (strcmp(argv[i], "--doctor") == 0) {
      doctor = 1;
      headless = 1;
    } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
      frames = strtoul(argv[++i], NULL, 10);
      headless = 1;
    } else if (strcmp(argv[i], "--dump") == 0 && i + 1 < argc) {
      dump = argv[++i];
      headless = 1;
    } else if (strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
      seconds = strtoul(argv[++i], NULL, 10);
    } else if (strcmp(argv[i], "--save") == 0 && i + 1 < argc) {
      save_option = argv[++i];
    } else if (argv[i][0] == '-' || filename != NULL) {
      return usage(argv[0]);
    } else {
      filename = argv[i];
    }
  }
  if (filename == NULL) {
    return usage(argv[0]);
  }

  uint64_t limit = UINT64_MAX;
  if (seconds) {
    limit = (uint64_t)seconds * CLOCK_HZ;
  } else if (headless) {
    limit = (uint64_t)DEFAULT_SECONDS * CLOCK_HZ;
  }

  if (!cart_load(filename)) {
    return EXIT_ERROR;
  }
  /* Gameboy Doctor's logs start from the DMG's registers */
  if (doctor && !gbc) {
    gb = 1;
  }
  if (cart_cgb() && !gb) {
    model = MODEL_CGB;
  } else if (gbc) {
    model = MODEL_CGB_DMG;
  }
  if (!mem_alloc()) {
    fprintf(stderr, "failboy: out of memory\n");
    cart_free();
    return EXIT_ERROR;
  }
  cpu_bios_init();

  /* Battery-backed RAM and the MBC3's clock are loaded now and saved again on the way out: the save file is --save's */
  /* or rom.sav next to the ROM, and the clock goes beside it with the same name, ending .rtc. */
  const char *save_file = save_option;
  char *default_save = NULL;
  char *rtc_file = NULL;
  if (save_file == NULL && (cart_battery_ram_size() > 0 || cart_has_rtc())) {
    default_save = with_extension(filename, SAVE_EXTENSION);
    save_file = default_save;
  }
  if (save_file != NULL && cart_has_rtc()) {
    rtc_file = with_extension(save_file, RTC_EXTENSION);
  }
  if (cart_battery_ram_size() == 0 || (save_file != NULL && !load_state(save_file, cart_write_battery_ram))) {
    save_file = NULL;
  }
  if (rtc_file != NULL && !load_state(rtc_file, cart_write_rtc)) {
    free(rtc_file);
    rtc_file = NULL;
  }

  int ok = 1;
  if (headless) {
    if (doctor) {
      setvbuf(stdout, NULL, _IOFBF, TRACE_BUFFER_SIZE);
      cpu_trace();
    }
    run_headless(limit, frames);
    if (dump) {
      ok = dump_frame(dump);
    }
  } else {
    ok = run_window(limit, rslcd);
  }

  if (save_file != NULL && !save_state(save_file, cart_battery_ram_size(), cart_read_battery_ram)) {
    ok = 0;
  }
  if (rtc_file != NULL && !save_state(rtc_file, CART_RTC_SIZE, cart_read_rtc)) {
    ok = 0;
  }
  free(default_save);
  free(rtc_file);

  int result = io_serial_result();
  mem_free();
  cart_free();

  if (!ok) {
    return EXIT_ERROR;
  }
  if (result == SERIAL_PASSED) {
    return EXIT_PASSED;
  }
  if (result == SERIAL_FAILED) {
    return EXIT_FAILED;
  }
  return EXIT_NO_RESULT;
}
