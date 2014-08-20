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
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdio.h>

#include "video_test_boom.h"

#define SCREEN_WIDTH	160
#define SCREEN_HEIGHT	144

SDL_Window *window;
SDL_Renderer *render;
SDL_PixelFormat *format;

SDL_Event event;

void msdl_create() {
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow(
				 "failboy",
				 SDL_WINDOWPOS_CENTERED,
				 SDL_WINDOWPOS_CENTERED,
				 SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN
			 );
	render = SDL_CreateRenderer(
				 window,
				 -1,
				 SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
			 );
	format = SDL_AllocFormat(SDL_GetWindowPixelFormat(window));
}

enum {
	PAL_DMG1 = 0,
	PAL_DMG2,
	PAL_GBP,
	PAL_GRAY,
	PAL_CATACLYSM
};

static SDL_Color palettes[5][4] = {
	{ {156, 189, 15, 255}, {48, 98, 48, 255}, {140, 173, 15, 255}, {15, 56, 15, 255} },
	{ {191, 197, 171, 255}, {98, 105, 73, 255}, {145, 154, 109, 255}, {53, 57, 40, 255} },
	{ {224, 224, 224, 255}, {96, 96, 96, 255}, {160, 160, 160, 255}, {32, 32, 32, 255} },
	{ {255, 255, 255, 255}, {85, 85, 85, 255}, {170, 170, 170, 255}, {0, 0, 0, 255} },
	{ {255, 247, 202, 255}, {146, 54, 12, 255}, {244, 194, 43, 255}, {16, 8, 0, 255} }
};

static SDL_Color *pal = palettes[PAL_CATACLYSM];

void msdl_render_tile(uint8_t *tile, uint8_t x, uint8_t y) {
	/*
	 * Each tile is 8 square. 16 bytes, and 2 bits per pixel. The upper bits are in the first byte,
	 * the lower bits in the second byte. So the MSB of each byte is the first pixel, and then so on.
	 */
	for(char row = 0; row < 8; ++row) {
		uint8_t *ptr = tile + (row << 1);
		uint8_t a = *ptr;
		uint8_t b = *(ptr + 1);
		/* do it backwards */
		for(char col = 7; col >= 0; --col) {
			/* just get the LSB of each */
			uint8_t v = ((a & 1) << 1) | (b & 1);
			SDL_SetRenderDrawColor(render, pal[v].r, pal[v].g, pal[v].b, 255);
			SDL_RenderDrawPoint(render, x + col, y + row);
			/* shift to the next msb bit of each */
			a >>= 1;
			b >>= 1;
		}
	}
}

void msdl_render() {
	/* basic gameboy tile renderer */
	int x = 0;
	int y = 0;
	for(int i = 0; i < gfx_tile_count; ++i) {
		uint8_t *ptr = (uint8_t *)gfx_tile_data + i * 16;
		msdl_render_tile(ptr, x, y);
		x += 8;
		if(x >= SCREEN_WIDTH) {
			x = 0;
			y += 8;
		}
	}
	/* todo render to a texture for... speed reasons. :D */
	SDL_RenderPresent(render);
}

void msdl_destroy() {
	SDL_FreeFormat(format);
	SDL_DestroyRenderer(render);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

int msdl_event() {
	while(SDL_WaitEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				return 0;
				/*
				case SDL_KEYDOWN:
				return sdl_event_key(event.key.keysym.sym);
				case SDL_TEXTINPUT:
				return *event.text.text; */
		}
	}
	return -1; /* notta */
}

int main(int argc, char *argv[]) {
	msdl_create();
	msdl_render();
	int event = 0;
	while((event = msdl_event())) {
		/* Do stuff here... */
	}
	msdl_destroy();
	return 0;
}
