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

#include "rtc.h"

#include <limits.h>
#include <string.h>
#include <time.h>

enum {
  RTC_SECONDS,
  RTC_MINUTES,
  RTC_HOURS,
  RTC_DAY_LOW,
  RTC_DAY_HIGH,
};

enum {
  DAY_HIGH_BIT = 0x01, /* bit 8 of the day counter */
  HALT = 0x40,
  DAY_CARRY = 0x80, /* the day counter overflowed; stays set until the game clears it */
  DAY_LOW_BITS = 8,
  DAYS = 512, /* where the day counter wraps */
  STATE_TIME = RTC_REGISTERS,
  TIME_BYTES = 8,
};

enum {
  SECONDS_PER_MINUTE = 60,
  MINUTES_PER_HOUR = 60,
  HOURS_PER_DAY = 24,
  SECONDS_PER_HOUR = SECONDS_PER_MINUTE * MINUTES_PER_HOUR,
  SECONDS_PER_DAY = SECONDS_PER_HOUR * HOURS_PER_DAY,
};

/* the bits each register really has */
static const uint8_t register_mask[RTC_REGISTERS] = {0x3F, 0x3F, 0x1F, 0xFF, 0xC1};

/* The registers as they were at host time `since`; while halted, just as they are. */
static uint8_t base[RTC_REGISTERS];
static int64_t since;

/* what the game reads: the registers at the last latch */
static uint8_t latched[RTC_REGISTERS];

static int64_t host_time(void) { return (int64_t)time(NULL); }

/* the registers as of now */
static void present(uint8_t *out) {
  memcpy(out, base, RTC_REGISTERS);
  int64_t elapsed = host_time() - since;
  if ((base[RTC_DAY_HIGH] & HALT) || elapsed <= 0) {
    return;
  }
  int64_t days = base[RTC_DAY_LOW] | (base[RTC_DAY_HIGH] & DAY_HIGH_BIT) << DAY_LOW_BITS;
  int64_t total = base[RTC_SECONDS] + base[RTC_MINUTES] * SECONDS_PER_MINUTE + base[RTC_HOURS] * SECONDS_PER_HOUR +
                  days * SECONDS_PER_DAY + elapsed;
  days = total / SECONDS_PER_DAY;
  out[RTC_SECONDS] = total % SECONDS_PER_MINUTE;
  out[RTC_MINUTES] = total / SECONDS_PER_MINUTE % MINUTES_PER_HOUR;
  out[RTC_HOURS] = total / SECONDS_PER_HOUR % HOURS_PER_DAY;
  if (days >= DAYS) {
    out[RTC_DAY_HIGH] |= DAY_CARRY;
    days %= DAYS;
  }
  out[RTC_DAY_LOW] = days;
  out[RTC_DAY_HIGH] = (out[RTC_DAY_HIGH] & ~DAY_HIGH_BIT) | days >> DAY_LOW_BITS;
}

void rtc_reset(void) {
  memset(base, 0, sizeof(base));
  memset(latched, 0, sizeof(latched));
  since = host_time();
}

void rtc_latch(void) { present(latched); }

uint8_t rtc_read(int reg) { return latched[reg]; }

void rtc_write(int reg, uint8_t value) {
  /* bring the clock up to now, and carry on from there with the new value */
  uint8_t now[RTC_REGISTERS];
  present(now);
  memcpy(base, now, sizeof(base));
  since = host_time();
  base[reg] = value & register_mask[reg];
}

void rtc_save(uint8_t *state) {
  memcpy(state, base, RTC_REGISTERS);
  uint64_t stamp = (uint64_t)since;
  for (int i = 0; i < TIME_BYTES; ++i) {
    state[STATE_TIME + i] = (uint8_t)(stamp >> (i * CHAR_BIT));
  }
}

void rtc_load(const uint8_t *state) {
  uint64_t stamp = 0;
  for (int i = 0; i < TIME_BYTES; ++i) {
    stamp |= (uint64_t)state[STATE_TIME + i] << (i * CHAR_BIT);
  }
  for (int reg = 0; reg < RTC_REGISTERS; ++reg) {
    base[reg] = state[reg] & register_mask[reg];
  }
  since = (int64_t)stamp;
  present(latched);
}
