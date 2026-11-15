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

#ifndef _TEST_H_
#define _TEST_H_

#include <stddef.h>
#include <stdint.h>

#include "failboy.h"

/* What the spot tests share. Each test is a program of its own that makes the cartridges it needs, so none of them */
/* wants a ROM file; its exit status is test_result(). */

enum {
  TEST_BANK_SIZE = 0x4000,
  TEST_ROM_SIZE = 2 * TEST_BANK_SIZE, /* the smallest cartridge */
  TEST_ENTRY = 0x0100,                /* where the boot ROM hands over, and test_program() puts a jump */
  TEST_PROGRAM = 0x0150,              /* where test_program() puts the program */
  TEST_HEADER_CGB = 0x0143,
  TEST_HEADER_CGB_GAME = 0x80, /* in TEST_HEADER_CGB: a color game */
};

#define TEST_HASH_START 0x811C9DC5u /* too big for an enum */

/* counts a check, and reports it if it failed */
#define CHECK(condition) test_check((condition) != 0, #condition, __FILE__, __LINE__)

void test_check(int ok, const char *what, const char *file, int line);
int test_result(const char *name); /* prints the count, and returns main()'s exit status: 0 if every check held */

/* A cartridge of size bytes, all zeros (NOPs) but for its header's type and RAM size, and each 16 kB bank's last two */
/* bytes, which hold the bank's number (low byte first) for test_bank() to read back. */
uint8_t *test_rom(uint32_t size, uint8_t type, uint8_t ram_size);
void test_program(uint8_t *rom, const uint8_t *code, uint32_t size); /* a program for it to run */
unsigned int test_bank(struct gb *gb, uint16_t address);             /* the bank the CPU sees at 0000 or 4000 */

/* a Game Boy with the cartridge in, which has to work */
struct gb *test_boot(enum gb_revision revision, const uint8_t *rom, uint32_t size, const struct gb_host *host);

/* FNV-1a, carried on from hash (TEST_HASH_START to begin with) */
uint32_t test_hash(const void *data, size_t size, uint32_t hash);

#endif /* _TEST_H_ */
