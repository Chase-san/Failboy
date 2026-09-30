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

#define CLOCK_HZ 4194304ull

enum { DEFAULT_SECONDS = 120, TRACE_BUFFER_SIZE = 1 << 20 };

/* exit status */
enum { EXIT_PASSED = 0, EXIT_FAILED = 1, EXIT_ERROR = 2, EXIT_NO_RESULT = 3 };

int doctor = 0;

static int usage(const char *name) {
  fprintf(stderr, "usage: %s [--doctor] [--seconds N] rom.gb\n", name);
  fprintf(stderr, "  --doctor     print a Gameboy Doctor trace to stdout (serial output goes to stderr)\n");
  fprintf(stderr, "  --seconds N  stop after N emulated seconds (default %d)\n", DEFAULT_SECONDS);
  fprintf(stderr, "exit status: %d passed, %d failed, %d error, %d no result\n", EXIT_PASSED, EXIT_FAILED, EXIT_ERROR,
          EXIT_NO_RESULT);
  return EXIT_ERROR;
}

int main(int argc, char *argv[]) {
  const char *filename = NULL;
  unsigned long seconds = DEFAULT_SECONDS;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--doctor") == 0) {
      doctor = 1;
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

  if (!cart_load(filename)) {
    return EXIT_ERROR;
  }
  if (!mem_alloc()) {
    fprintf(stderr, "failboy: out of memory\n");
    cart_free();
    return EXIT_ERROR;
  }
  cpu_bios_init();

  if (doctor) {
    setvbuf(stdout, NULL, _IOFBF, TRACE_BUFFER_SIZE);
    cpu_trace();
  }

  uint64_t limit = seconds * CLOCK_HZ;
  while (!cpu_locked && cycle_counter < limit) {
    uint16_t pc = r.PC;
    step();
    /* Jumping to itself with interrupts off: nothing can ever get it out (test ROMs end this way). */
    if (r.PC == pc && !halted && !ime) {
      break;
    }
    if (doctor && ferror(stdout)) {
      break;
    }
  }
  fflush(stdout);

  int result = io_serial_result();
  mem_free();
  cart_free();

  if (result == SERIAL_PASSED) {
    return EXIT_PASSED;
  }
  if (result == SERIAL_FAILED) {
    return EXIT_FAILED;
  }
  return EXIT_NO_RESULT;
}
