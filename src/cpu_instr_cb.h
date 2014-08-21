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

#ifndef _CPU_INSTR_CB_H_
#define _CPU_INSTR_CB_H_

/* This file defines the DMG two byte 0xCB instructions. */
void SWAP_A();
void SWAP_B();
void SWAP_C();
void SWAP_D();
void SWAP_E();
void SWAP_H();
void SWAP_L();
void SWAP_aHL();

#endif
