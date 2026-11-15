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

#include "tests/test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  HEADER_CART_TYPE = 0x0147,
  HEADER_RAM_SIZE = 0x0149,
  OP_NOP = 0x00,
  OP_JP = 0xC3,
  BYTE_BITS = 8,
  BYTE_MASK = 0xFF,
  HASH_PRIME = 0x01000193,
};

static int checks = 0;
static int failures = 0;

void test_check(int ok, const char *what, const char *file, int line) {
  ++checks;
  if (!ok) {
    ++failures;
    printf("%s:%d: failed: %s\n", file, line, what);
  }
}

int test_result(const char *name) {
  printf("%s: %d checks, %d failed\n", name, checks, failures);
  return failures != 0;
}

uint8_t *test_rom(uint32_t size, uint8_t type, uint8_t ram_size) {
  uint8_t *rom = calloc(1, size);
  if (rom == NULL) {
    printf("out of memory\n");
    exit(EXIT_FAILURE);
  }
  rom[HEADER_CART_TYPE] = type;
  rom[HEADER_RAM_SIZE] = ram_size;
  for (uint32_t bank = 0; bank < size / TEST_BANK_SIZE; ++bank) {
    uint8_t *end = rom + (bank + 1) * TEST_BANK_SIZE;
    end[-2] = bank & BYTE_MASK;
    end[-1] = bank >> BYTE_BITS;
  }
  return rom;
}

void test_program(uint8_t *rom, const uint8_t *code, uint32_t size) {
  rom[TEST_ENTRY] = OP_NOP;
  rom[TEST_ENTRY + 1] = OP_JP;
  rom[TEST_ENTRY + 2] = TEST_PROGRAM & BYTE_MASK;
  rom[TEST_ENTRY + 3] = TEST_PROGRAM >> BYTE_BITS;
  memcpy(rom + TEST_PROGRAM, code, size);
}

unsigned int test_bank(struct gb *gb, uint16_t address) {
  uint16_t end = address + TEST_BANK_SIZE;
  return gb_read(gb, end - 2) | gb_read(gb, end - 1) << BYTE_BITS;
}

struct gb *test_boot(enum gb_revision revision, const uint8_t *rom, uint32_t size, const struct gb_host *host) {
  struct gb_config config = {0};
  struct gb *gb = NULL;
  config.revision = revision;
  config.rom = rom;
  config.rom_size = size;
  if (host != NULL) {
    config.host = *host;
  }
  enum gb_error error = gb_create(&config, &gb);
  if (error != GB_OK || gb == NULL) {
    printf("gb_create() failed with %d\n", (int)error);
    exit(EXIT_FAILURE);
  }
  return gb;
}

uint32_t test_hash(const void *data, size_t size, uint32_t hash) {
  const uint8_t *bytes = data;
  for (size_t i = 0; i < size; ++i) {
    hash = (hash ^ bytes[i]) * HASH_PRIME;
  }
  return hash;
}
