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

#include "core.h"

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
  STATE_TIME = RTC_REGISTERS,
  TIME_BYTES = 8,
};

enum {
  SECONDS_PER_MINUTE = 60,
  MINUTES_PER_HOUR = 60,
  HOURS_PER_DAY = 24,
  SECONDS_PER_DAY = SECONDS_PER_MINUTE * MINUTES_PER_HOUR * HOURS_PER_DAY,
  DAY_LOW_TOP = 0xFF,
  /* Where in its second the clock is when nothing says: a real one's crystal runs on with the power off, so it could */
  /* be anywhere, and halfway is as good a guess as the start. */
  UNKNOWN_FRACTION = GB_CLOCK_HZ / 2,
};

/* the bits each register really has */
static const uint8_t register_mask[RTC_REGISTERS] = {0x3F, 0x3F, 0x1F, 0xFF, 0xC1};

static uint8_t now[RTC_REGISTERS];     /* the clock's counters */
static uint32_t fraction;              /* T-cycles into the present second */
static uint8_t latched[RTC_REGISTERS]; /* what the game reads: the counters at the last latch */

/* A day on: the 9-bit day counter comes round to 0 with the carry flag left set. */
static void day(void) {
  if (now[RTC_DAY_LOW] != DAY_LOW_TOP) {
    ++now[RTC_DAY_LOW];
    return;
  }
  now[RTC_DAY_LOW] = 0;
  if (now[RTC_DAY_HIGH] & DAY_HIGH_BIT) {
    now[RTC_DAY_HIGH] = (now[RTC_DAY_HIGH] & ~DAY_HIGH_BIT) | DAY_CARRY;
  } else {
    now[RTC_DAY_HIGH] |= DAY_HIGH_BIT;
  }
}

/* A second on. Each counter is as wide as its register and carries only from its proper top: one written past that */
/* (seconds of 60 to 63, say) counts on to the end of its bits and comes round to 0 without carrying. */
static void second(void) {
  now[RTC_SECONDS] = (now[RTC_SECONDS] + 1) & register_mask[RTC_SECONDS];
  if (now[RTC_SECONDS] != SECONDS_PER_MINUTE) {
    return;
  }
  now[RTC_SECONDS] = 0;
  now[RTC_MINUTES] = (now[RTC_MINUTES] + 1) & register_mask[RTC_MINUTES];
  if (now[RTC_MINUTES] != MINUTES_PER_HOUR) {
    return;
  }
  now[RTC_MINUTES] = 0;
  now[RTC_HOURS] = (now[RTC_HOURS] + 1) & register_mask[RTC_HOURS];
  if (now[RTC_HOURS] != HOURS_PER_DAY) {
    return;
  }
  now[RTC_HOURS] = 0;
  day();
}

static int stopped_clock(void) { return (now[RTC_DAY_HIGH] & HALT) != 0; }

/* The clock runs on the machine's own time while it's on. */
void rtc_advance(uint32_t ticks) {
  if (stopped_clock()) {
    return;
  }
  fraction += ticks;
  while (fraction >= GB_CLOCK_HZ) {
    fraction -= GB_CLOCK_HZ;
    second();
  }
}

/* The time the power was off for, which only the host knows. */
static void elapse(int64_t seconds) {
  if (stopped_clock() || seconds <= 0) {
    return;
  }
  for (int64_t days = seconds / SECONDS_PER_DAY; days > 0; --days) {
    day();
  }
  for (int64_t rest = seconds % SECONDS_PER_DAY; rest > 0; --rest) {
    second();
  }
}

void rtc_reset(void) {
  memset(now, 0, sizeof(now));
  memset(latched, 0, sizeof(latched));
  fraction = UNKNOWN_FRACTION;
}

void rtc_latch(void) { memcpy(latched, now, sizeof(latched)); }

uint8_t rtc_read(int reg) { return latched[reg]; }

/* Setting the seconds starts the second over. */
void rtc_write(int reg, uint8_t value) {
  now[reg] = value & register_mask[reg];
  if (reg == RTC_SECONDS) {
    fraction = 0;
  }
}

void rtc_save(uint8_t *state) {
  memcpy(state, now, RTC_REGISTERS);
  uint64_t stamp = (uint64_t)host_clock();
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
    now[reg] = state[reg] & register_mask[reg];
  }
  fraction = UNKNOWN_FRACTION;
  if (stamp != 0) { /* saved with a host clock to go by */
    elapse(host_clock() - (int64_t)stamp);
  }
  rtc_latch();
}
