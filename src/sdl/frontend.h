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

#ifndef _FRONTEND_H_
#define _FRONTEND_H_

#include <stdint.h>

/* Runs the loaded cartridge in a window until it's closed, or for up to cycle_limit T-cycles, starting in RSLCD */
/* mode if rslcd is set. Returns 0 if the window couldn't be opened. */
int frontend_run(uint64_t, int);

#endif /* _FRONTEND_H_ */
