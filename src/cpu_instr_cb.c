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

/* **************************************** */
/* CB Instructions */
static inline uint8_t SWAP(uint8_t n) {
	r.F = 0;
	r.F_Z = n == 0;
	return (n >> 4) | (n << 4);
}
void SWAP_A() { r.A = SWAP(r.A); }
void SWAP_B() { r.B = SWAP(r.B); }
void SWAP_C() { r.C = SWAP(r.C); }
void SWAP_D() { r.D = SWAP(r.D); }
void SWAP_E() { r.E = SWAP(r.E); }
void SWAP_H() { r.H = SWAP(r.H); }
void SWAP_L() { r.L = SWAP(r.L); }
void SWAP_aHL() { write(r.HL, SWAP(read(r.HL))); }
