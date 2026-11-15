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

/* The command line: loads a ROM and its saves, and runs it in a window, or headless for the test ROMs. */

#include "failboy.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cli/files.h"

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
  HEADER_CART_TYPE = 0x147,
  BYTE_BITS = 8,
  BYTE_MASK = 0xFF,
  TRACE_BYTES = 4, /* of memory at PC, in each line of a trace */
};

/* exit status */
enum {
  EXIT_PASSED = 0,
  EXIT_FAILED = 1,
  EXIT_ERROR = 2,
  EXIT_NO_RESULT = 3,
};

/* what Blargg's test ROMs have reported over the serial port */
enum {
  SERIAL_NONE,
  SERIAL_PASSED,
  SERIAL_FAILED,
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

static const char UNSUPPORTED_MODE[] = "sgb"; /* a --mode the core can't be yet */
static const char SAVE_EXTENSION[] = ".sav";
static const char RTC_EXTENSION[] = ".rtc";

/* --mode: the machine to run as, or what the cartridge's header asks for */
static const struct {
  const char *name;
  enum gb_revision revision;
} modes[] = {
    {"auto", GB_AUTO},
    {"dmg", GB_DMG_AUTO},
    {"cgb", GB_CGB_AUTO},
    {"cgbfb", GB_CGB_AUTO}, /* once a mode of its own; a cgb falls back by itself now */
};

/* --doctor: a Gameboy Doctor trace on stdout, which moves serial output to stderr */
static int doctor = 0;

/* The last few characters sent over serial, to spot Blargg's "Passed"/"Failed". */
static char serial_tail[6];
static int serial_result = SERIAL_NONE;

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
  fprintf(stderr, "  --mode MODE  mode override: default is auto\n");
  fprintf(stderr, "         auto  select based on the game\n");
  fprintf(stderr, "         dmg   original gameboy\n");
  fprintf(stderr, "         cgb   color gameboy, which runs an original gameboy game in its fallback mode\n");
  fprintf(stderr, "         sgb   super gameboy (unimplemented)\n");
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

/* the index in modes of a --mode, or -1 */
static int find_mode(const char *name) {
  for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
    if (strcmp(name, modes[i].name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

/* **************************************** */
/* What the core calls back for */

static void serial_out(struct gb *gb, void *user, uint8_t c) {
  (void)gb;
  (void)user;
  FILE *out = stdout;
  if (doctor) {
    /* stdout is carrying the trace */
    out = stderr;
  }
  fputc(c, out);
  fflush(out);

  memmove(serial_tail, serial_tail + 1, sizeof(serial_tail) - 1);
  serial_tail[sizeof(serial_tail) - 1] = c;
  if (memcmp(serial_tail, "Failed", sizeof(serial_tail)) == 0) {
    serial_result = SERIAL_FAILED;
  } else if (serial_result == SERIAL_NONE && memcmp(serial_tail, "Passed", sizeof(serial_tail)) == 0) {
    serial_result = SERIAL_PASSED;
  }
}

/* One line per instruction, in Gameboy Doctor's log format. */
static void trace(struct gb *gb, void *user) {
  (void)user;
  struct gb_cpu cpu;
  uint8_t memory[TRACE_BYTES];
  gb_cpu(gb, &cpu);
  for (int i = 0; i < TRACE_BYTES; ++i) {
    memory[i] = gb_read(gb, (uint16_t)(cpu.pc + i));
  }
  printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
         cpu.af >> BYTE_BITS, cpu.af & BYTE_MASK, cpu.bc >> BYTE_BITS, cpu.bc & BYTE_MASK, cpu.de >> BYTE_BITS,
         cpu.de & BYTE_MASK, cpu.hl >> BYTE_BITS, cpu.hl & BYTE_MASK, cpu.sp, cpu.pc, memory[0], memory[1], memory[2],
         memory[3]);
}

/* a cartridge's clock keeps the real time */
static int64_t host_time(struct gb *gb, void *user) {
  (void)gb;
  (void)user;
  return (int64_t)time(NULL);
}

/* **************************************** */

static int dump_frame(const struct gb *gb, const char *filename) {
  FILE *f = fopen(filename, "wb");
  if (f == NULL) {
    fprintf(stderr, "failboy: can't write '%s'\n", filename);
    return 0;
  }
  const uint16_t *frame = gb_pixels(gb);
  if (gb_pixel_format(gb) == GB_PIXELS_SHADES) {
    fprintf(f, "P5\n%d %d\n255\n", GB_LCD_WIDTH, GB_LCD_HEIGHT);
    for (int i = 0; i < GB_LCD_WIDTH * GB_LCD_HEIGHT; ++i) {
      fputc(dump_gray[frame[i]], f);
    }
  } else {
    fprintf(f, "P6\n%d %d\n255\n", GB_LCD_WIDTH, GB_LCD_HEIGHT);
    for (int i = 0; i < GB_LCD_WIDTH * GB_LCD_HEIGHT; ++i) {
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

static void run_headless(struct gb *gb, uint64_t limit, unsigned long frames) {
  struct gb_cpu cpu;
  gb_cpu(gb, &cpu);
  while (!cpu.locked && gb_ticks(gb) < limit) {
    uint16_t pc = cpu.pc;
    gb_step(gb);
    gb_cpu(gb, &cpu);
    if (frames) {
      if (gb_frames(gb) >= frames) {
        break;
      }
    } else if (cpu.pc == pc && !cpu.halted && !cpu.ime) {
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
static int load_state(struct gb *gb, const char *path, void (*restore)(struct gb *, const uint8_t *, uint32_t)) {
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
  restore(gb, data, size);
  free(data);
  return 1;
}

static int save_state(const struct gb *gb, const char *path, uint32_t size,
                      void (*collect)(const struct gb *, uint8_t *, uint32_t)) {
  uint8_t *data = malloc(size);
  int ok = data != NULL;
  if (ok) {
    collect(gb, data, size);
    ok = file_save(path, data, size);
  }
  if (!ok) {
    fprintf(stderr, "failboy: can't write the save file '%s'\n", path);
  }
  free(data);
  return ok;
}

static int run_window(struct gb *gb, uint64_t limit, int rslcd) {
#ifdef FAILBOY_SDL
  return frontend_run(gb, limit, rslcd);
#else
  (void)gb;
  (void)limit;
  (void)rslcd;
  return 0;
#endif
}

/* why gb_create() wouldn't */
static void report(enum gb_error error, const char *filename, const uint8_t *rom, unsigned int rom_size) {
  switch (error) {
    case GB_ERROR_ROM:
      fprintf(stderr, "failboy: '%s' isn't a Game Boy ROM (size %u)\n", filename, rom_size);
      break;
    case GB_ERROR_CARTRIDGE:
      fprintf(stderr, "failboy: '%s' uses cartridge type %02X, which isn't supported yet\n", filename,
              rom[HEADER_CART_TYPE]);
      break;
    case GB_ERROR_MEMORY:
      fprintf(stderr, "failboy: out of memory\n");
      break;
    default:
      fprintf(stderr, "failboy: the core won't start (error %d)\n", (int)error);
      break;
  }
}

int main(int argc, char *argv[]) {
  const char *filename = NULL;
  const char *dump = NULL;
  const char *save_option = NULL;
  unsigned long seconds = 0;
  unsigned long frames = 0;
  int headless = 1;
  int rslcd = 0;
  int mode = 0; /* auto */
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
    } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
      ++i;
      if (strcmp(argv[i], UNSUPPORTED_MODE) == 0) {
        fprintf(stderr, "failboy: %s mode isn't supported yet\n", UNSUPPORTED_MODE);
        return EXIT_ERROR;
      }
      mode = find_mode(argv[i]);
      if (mode < 0) {
        return usage(argv[0]);
      }
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
    limit = (uint64_t)seconds * GB_CLOCK_HZ;
  } else if (headless) {
    limit = (uint64_t)DEFAULT_SECONDS * GB_CLOCK_HZ;
  }

  unsigned int rom_size = 0;
  uint8_t *rom = file_load(filename, &rom_size);
  if (rom == NULL) {
    fprintf(stderr, "failboy: can't read '%s'\n", filename);
    return EXIT_ERROR;
  }

  struct gb_config config = {0};
  config.revision = modes[mode].revision;
  /* Gameboy Doctor's logs start from the DMG's registers */
  if (doctor && config.revision == GB_AUTO) {
    config.revision = GB_DMG_AUTO;
  }
  config.rom = rom;
  config.rom_size = rom_size;
  config.doctor = doctor;
  config.host.serial = serial_out;
  config.host.clock = host_time;
  if (doctor) {
    config.host.trace = trace;
  }
  struct gb *gb = NULL;
  enum gb_error error = gb_create(&config, &gb);
  if (error != GB_OK) {
    report(error, filename, rom, rom_size);
    free(rom);
    return EXIT_ERROR;
  }

  /* Battery-backed RAM and the MBC3's clock are loaded now and saved again on the way out: the save file is --save's */
  /* or rom.sav next to the ROM, and the clock goes beside it with the same name, ending .rtc. */
  const char *save_file = save_option;
  char *default_save = NULL;
  char *rtc_file = NULL;
  if (save_file == NULL && (gb_save_size(gb) > 0 || gb_has_rtc(gb))) {
    default_save = with_extension(filename, SAVE_EXTENSION);
    save_file = default_save;
  }
  if (save_file != NULL && gb_has_rtc(gb)) {
    rtc_file = with_extension(save_file, RTC_EXTENSION);
  }
  if (gb_save_size(gb) == 0 || (save_file != NULL && !load_state(gb, save_file, gb_save_write))) {
    save_file = NULL;
  }
  if (rtc_file != NULL && !load_state(gb, rtc_file, gb_rtc_write)) {
    free(rtc_file);
    rtc_file = NULL;
  }

  int ok = 1;
  if (headless) {
    if (doctor) {
      setvbuf(stdout, NULL, _IOFBF, TRACE_BUFFER_SIZE);
      trace(gb, NULL);
    }
    run_headless(gb, limit, frames);
    if (dump) {
      ok = dump_frame(gb, dump);
    }
  } else {
    ok = run_window(gb, limit, rslcd);
  }
  struct gb_cpu cpu;
  gb_cpu(gb, &cpu);
  if (cpu.locked) {
    uint16_t pc = cpu.pc - 1;
    fprintf(stderr, "illegal opcode %02X at %04X\n", gb_read(gb, pc), pc);
  }

  if (save_file != NULL && !save_state(gb, save_file, gb_save_size(gb), gb_save_read)) {
    ok = 0;
  }
  if (rtc_file != NULL && !save_state(gb, rtc_file, GB_RTC_SIZE, gb_rtc_read)) {
    ok = 0;
  }
  free(default_save);
  free(rtc_file);
  gb_destroy(gb);
  free(rom);

  if (!ok) {
    return EXIT_ERROR;
  }
  if (serial_result == SERIAL_PASSED) {
    return EXIT_PASSED;
  }
  if (serial_result == SERIAL_FAILED) {
    return EXIT_FAILED;
  }
  return EXIT_NO_RESULT;
}
