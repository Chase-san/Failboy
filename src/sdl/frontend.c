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

/* SDL3 front end: a window showing each finished frame, and the keyboard as a joypad. */

#include "sdl/frontend.h"

#include <SDL3/SDL.h>
#include <stdio.h>

/* main() stays ours; with this, SDL_main.h only declares SDL_SetMainReady() */
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>

#include "failboy.h"

enum {
  WINDOW_SCALE = 4,
  MAX_LAG_FRAMES = 4, /* further behind than this (a dragged window, a debugger) and we stop trying to catch up */
};

/* XRGB8888 for shades 0 (lightest) to 3 */
static const uint32_t shade_rgb[4] = {
    0xE0F8D0,
    0x88C070,
    0x346856,
    0x081820,
};

static const struct {
  SDL_Scancode key;
  uint8_t button;
} keymap[] = {
    {SDL_SCANCODE_RIGHT, JOYPAD_RIGHT},      {SDL_SCANCODE_LEFT, JOYPAD_LEFT},    {SDL_SCANCODE_UP, JOYPAD_UP},
    {SDL_SCANCODE_DOWN, JOYPAD_DOWN},        {SDL_SCANCODE_X, JOYPAD_A},          {SDL_SCANCODE_Z, JOYPAD_B},
    {SDL_SCANCODE_BACKSPACE, JOYPAD_SELECT}, {SDL_SCANCODE_RETURN, JOYPAD_START},
};

static uint8_t read_buttons(void) {
  const bool *keys = SDL_GetKeyboardState(NULL);
  uint8_t pressed = 0;
  for (size_t i = 0; i < sizeof(keymap) / sizeof(keymap[0]); ++i) {
    if (keys[keymap[i].key]) {
      pressed |= keymap[i].button;
    }
  }
  return pressed;
}

/* Runs until the PPU finishes a frame, but no longer than a frame's worth, so the window stays live while the LCD */
/* is off. */
static void run_frame(void) {
  uint32_t frame = video_frames();
  uint32_t cycles = 0;
  while (!cpu_locked && video_frames() == frame && cycles < FRAME_CYCLES) {
    cycles += step();
  }
}

static void present(SDL_Renderer *renderer, SDL_Texture *texture) {
  void *pixels;
  int pitch;
  if (SDL_LockTexture(texture, NULL, &pixels, &pitch)) {
    const uint8_t *frame = video_framebuffer();
    for (int y = 0; y < LCD_HEIGHT; ++y) {
      uint32_t *row = (uint32_t *)((uint8_t *)pixels + y * pitch);
      for (int x = 0; x < LCD_WIDTH; ++x) {
        row[x] = shade_rgb[frame[y * LCD_WIDTH + x]];
      }
    }
    SDL_UnlockTexture(texture);
  }
  SDL_RenderClear(renderer);
  SDL_RenderTexture(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

int frontend_run(uint64_t cycle_limit) {
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  SDL_Texture *texture = NULL;

  /* main() is ours, not SDL_main's */
  SDL_SetMainReady();
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "failboy: can't start SDL: %s\n", SDL_GetError());
    return 0;
  }
  if (SDL_CreateWindowAndRenderer("Failboy", LCD_WIDTH * WINDOW_SCALE, LCD_HEIGHT * WINDOW_SCALE, SDL_WINDOW_RESIZABLE,
                                  &window, &renderer)) {
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, LCD_WIDTH, LCD_HEIGHT);
  }
  if (texture == NULL) {
    fprintf(stderr, "failboy: can't open a window: %s\n", SDL_GetError());
    SDL_Quit();
    return 0;
  }
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
  SDL_SetRenderLogicalPresentation(renderer, LCD_WIDTH, LCD_HEIGHT, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

  /* a frame lasts 70224 T-cycles at 4.19 MHz, about 16.74 ms (59.73 Hz) */
  const uint64_t frame_ns = (uint64_t)FRAME_CYCLES * SDL_NS_PER_SECOND / CLOCK_HZ;
  uint64_t next = SDL_GetTicksNS();
  int running = 1;
  while (running && cycle_counter < cycle_limit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = 0;
      } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) {
        running = 0;
      }
    }
    io_joypad(read_buttons());
    run_frame();
    present(renderer, texture);

    next += frame_ns;
    uint64_t now = SDL_GetTicksNS();
    if (now < next) {
      SDL_DelayPrecise(next - now);
    } else if (now - next > frame_ns * MAX_LAG_FRAMES) {
      next = now;
    }
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 1;
}
