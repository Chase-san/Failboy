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

#ifndef _RTC_H_
#define _RTC_H_

#include <stdint.h>

/* The MBC3's real-time clock, done cheaply: it remembers the registers as they were at some moment of host time, */
/* and works out the present from the host clock whenever the game latches it, so it never has to tick. */

enum {
  RTC_REGISTERS = 5,   /* seconds, minutes, hours, day counter low, then day counter high and flags */
  RTC_STATE_SIZE = 13, /* those registers, then the host time they were right at (8 bytes, little-endian) */
};

void rtc_reset(void);
void rtc_latch(void);         /* copies the present into the registers the game reads */
uint8_t rtc_read(int);        /* a latched register, 0 to RTC_REGISTERS - 1 */
void rtc_write(int, uint8_t); /* sets the clock */
void rtc_save(uint8_t *);     /* RTC_STATE_SIZE bytes */
void rtc_load(const uint8_t *);

#endif /* _RTC_H_ */
