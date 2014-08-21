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
#include "files.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
	cart_load("tests/cpu_instrs.gb");
	mem_alloc();
	cpu_init();
	for(uint32_t i = 0; i < 4294967295; ++i) {
		exec();
	}
	printf("\n\nEND OF LINE\n");
	mem_free();
	cart_free();
	return 0;
}
