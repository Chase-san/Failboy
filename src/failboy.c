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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef FAILBOY_SDL
#include "sdl/frontend.h"
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

/* --dump writes an 8-bit greyscale PGM, with the DMG's four shades */
static const uint8_t dump_gray[4] = {0xFF, 0xAA, 0x55, 0x00};

int doctor = 0;

static int usage(const char *name) {
  fprintf(stderr, "usage: %s [options] rom.gb\n", name);
#ifdef FAILBOY_SDL
  fprintf(stderr, "  --headless   run without a window (implied by the options below)\n");
#endif
  fprintf(stderr, "  --doctor     print a Gameboy Doctor trace to stdout (serial output goes to stderr)\n");
  fprintf(stderr, "  --frames N   stop after N frames\n");
  fprintf(stderr, "  --dump FILE  save the last frame as a PGM image\n");
  fprintf(stderr, "  --seconds N  stop after N emulated seconds (headless default %d)\n", DEFAULT_SECONDS);
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
  const uint8_t *frame = video_framebuffer();
  fprintf(f, "P5\n%d %d\n255\n", LCD_WIDTH, LCD_HEIGHT);
  for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; ++i) {
    fputc(dump_gray[frame[i]], f);
  }
  if (fclose(f) != 0) {
    fprintf(stderr, "failboy: can't write '%s'\n", filename);
    return 0;
  }
  return 1;
}

static void run_headless(uint64_t limit, unsigned long frames) {
  while (!cpu_locked && cycle_counter < limit) {
    uint16_t pc = r.PC;
    step();
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

static int run_window(uint64_t limit) {
#ifdef FAILBOY_SDL
  return frontend_run(limit);
#else
  (void)limit;
  return 0;
#endif
}

int main(int argc, char *argv[]) {
  const char *filename = NULL;
  const char *dump = NULL;
  unsigned long seconds = 0;
  unsigned long frames = 0;
  int headless = 1;
#ifdef FAILBOY_SDL
  headless = 0;
#endif

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--headless") == 0) {
      headless = 1;
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
  if (!mem_alloc()) {
    fprintf(stderr, "failboy: out of memory\n");
    cart_free();
    return EXIT_ERROR;
  }
  cpu_bios_init();

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
    ok = run_window(limit);
  }

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
