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

/* SDL3 front end: a window showing each finished frame, the APU's samples on the default audio device, and the */
/* keyboard as a joypad. */
/*   arrows = D-pad, X = A, Z = B, Enter = Start, Backspace = Select, Esc = quit */
/*   L = RSLCD on/off */

#include "sdl/frontend.h"

#include <SDL3/SDL.h>
#include <stdio.h>

/* main() stays ours; with this, SDL_main.h only declares SDL_SetMainReady() */
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>

#include "failboy.h"
#include "sdl/rslcd.h"

enum {
  WINDOW_SCALE = 4,
  MAX_LAG_FRAMES = 4, /* further behind than this (a dragged window, a debugger) and we stop trying to catch up */
  AUDIO_CHANNELS = 2,
  AUDIO_FRAME_BYTES = AUDIO_CHANNELS * sizeof(int16_t),
  AUDIO_TARGET = AUDIO_RATE / 20,     /* sample frames to keep queued for the device: 50 ms */
  AUDIO_MAX_QUEUE = AUDIO_TARGET * 4, /* further ahead than this, samples are dropped rather than add to the delay */
};

/* The most the playback rate is bent to hold the queue at AUDIO_TARGET: 0.5%, too little to hear. */
static const float AUDIO_MAX_SKEW = 0.005f;

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

struct screen {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *plain; /* LCD_WIDTH x LCD_HEIGHT */
  SDL_Texture *lcd;   /* RSLCD, at lcd_scale output pixels per Game Boy pixel */
  int lcd_scale;
  int rslcd;
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

/* SDL converts the APU's samples for the audio device. Returns NULL, and we carry on without sound, if it can't. */
static SDL_AudioStream *open_audio(void) {
  const SDL_AudioSpec spec = {SDL_AUDIO_S16, AUDIO_CHANNELS, AUDIO_RATE};
  SDL_AudioStream *stream = NULL;
  if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
  }
  if (stream == NULL) {
    fprintf(stderr, "failboy: no sound: %s\n", SDL_GetError());
  }
  return stream;
}

/* Queues the frame's samples. The device's clock never quite matches the one pacing the frames, so the playback */
/* rate is bent slightly to hold the queue at AUDIO_TARGET, which also covers the device taking big bites of it. */
static void play_audio(SDL_AudioStream *stream) {
  unsigned int frames = 0;
  const int16_t *samples = audio_samples(&frames);
  if (stream == NULL) {
    return;
  }
  int queued = SDL_GetAudioStreamQueued(stream) / AUDIO_FRAME_BYTES;
  if (queued == 0) {
    /* it ran dry (at the start, or after the window was dragged): wait for a full queue again before playing on */
    SDL_PauseAudioStreamDevice(stream);
  }
  if (queued > AUDIO_MAX_QUEUE) {
    return;
  }
  SDL_PutAudioStreamData(stream, samples, (int)(frames * AUDIO_FRAME_BYTES));
  queued += (int)frames;
  if (queued >= AUDIO_TARGET && SDL_AudioStreamDevicePaused(stream)) {
    SDL_ResumeAudioStreamDevice(stream);
  }

  float skew = (float)(queued - AUDIO_TARGET) / AUDIO_TARGET;
  if (skew > 1) {
    skew = 1;
  }
  if (skew < -1) {
    skew = -1;
  }
  SDL_SetAudioStreamFrequencyRatio(stream, 1 + skew * AUDIO_MAX_SKEW);
}

static void update_title(struct screen *screen) {
  const char *title = "Failboy";
  if (screen->rslcd) {
    title = "Failboy (RSLCD)";
  }
  SDL_SetWindowTitle(screen->window, title);
}

/* The front end's own keys; returns 0 to quit. */
static int handle_key(struct screen *screen, SDL_Scancode key) {
  switch (key) {
    case SDL_SCANCODE_ESCAPE:
      return 0;
    case SDL_SCANCODE_L:
      screen->rslcd = !screen->rslcd;
      if (screen->rslcd) {
        rslcd_reset(video_framebuffer());
      }
      update_title(screen);
      return 1;
    default:
      return 1;
  }
}

static void draw_plain(struct screen *screen) {
  void *pixels;
  int pitch;
  if (SDL_LockTexture(screen->plain, NULL, &pixels, &pitch)) {
    const uint8_t *frame = video_framebuffer();
    for (int y = 0; y < LCD_HEIGHT; ++y) {
      uint32_t *row = (uint32_t *)((uint8_t *)pixels + y * pitch);
      for (int x = 0; x < LCD_WIDTH; ++x) {
        row[x] = shade_rgb[frame[y * LCD_WIDTH + x]];
      }
    }
    SDL_UnlockTexture(screen->plain);
  }
  SDL_SetRenderLogicalPresentation(screen->renderer, LCD_WIDTH, LCD_HEIGHT, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
  SDL_RenderClear(screen->renderer);
  SDL_RenderTexture(screen->renderer, screen->plain, NULL, NULL);
}

static void draw_rslcd(struct screen *screen) {
  /* The biggest whole number of output pixels per Game Boy pixel that fits, drawn 1:1 so the grid stays crisp. */
  int width = 0;
  int height = 0;
  SDL_GetRenderOutputSize(screen->renderer, &width, &height);
  int scale = width / LCD_WIDTH;
  if (height / LCD_HEIGHT < scale) {
    scale = height / LCD_HEIGHT;
  }
  if (scale < 1) {
    scale = 1;
  }
  if (scale > RSLCD_MAX_SCALE) {
    scale = RSLCD_MAX_SCALE;
  }
  if (scale != screen->lcd_scale) {
    if (screen->lcd != NULL) {
      SDL_DestroyTexture(screen->lcd);
    }
    screen->lcd = SDL_CreateTexture(screen->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
                                    LCD_WIDTH * scale, LCD_HEIGHT * scale);
    screen->lcd_scale = 0;
    if (screen->lcd == NULL) {
      return;
    }
    SDL_SetTextureScaleMode(screen->lcd, SDL_SCALEMODE_NEAREST);
    screen->lcd_scale = scale;
  }

  rslcd_frame(video_framebuffer());
  void *pixels;
  int pitch;
  if (SDL_LockTexture(screen->lcd, NULL, &pixels, &pitch)) {
    rslcd_draw(scale, pixels, pitch / (int)sizeof(uint32_t));
    SDL_UnlockTexture(screen->lcd);
  }
  SDL_SetRenderLogicalPresentation(screen->renderer, LCD_WIDTH * scale, LCD_HEIGHT * scale,
                                   SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
  SDL_RenderClear(screen->renderer);
  SDL_RenderTexture(screen->renderer, screen->lcd, NULL, NULL);
}

int frontend_run(uint64_t cycle_limit, int rslcd) {
  struct screen screen = {0};
  screen.rslcd = rslcd;

  /* main() is ours, not SDL_main's */
  SDL_SetMainReady();
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "failboy: can't start SDL: %s\n", SDL_GetError());
    return 0;
  }
  if (SDL_CreateWindowAndRenderer("Failboy", LCD_WIDTH * WINDOW_SCALE, LCD_HEIGHT * WINDOW_SCALE, SDL_WINDOW_RESIZABLE,
                                  &screen.window, &screen.renderer)) {
    screen.plain = SDL_CreateTexture(screen.renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, LCD_WIDTH,
                                     LCD_HEIGHT);
  }
  if (screen.plain == NULL) {
    fprintf(stderr, "failboy: can't open a window: %s\n", SDL_GetError());
    SDL_Quit();
    return 0;
  }
  SDL_SetTextureScaleMode(screen.plain, SDL_SCALEMODE_NEAREST);
  update_title(&screen);
  if (screen.rslcd) {
    rslcd_reset(video_framebuffer());
  }
  SDL_AudioStream *audio = open_audio();

  /* a frame lasts 70224 T-cycles at 4.19 MHz, about 16.74 ms (59.73 Hz) */
  const uint64_t frame_ns = (uint64_t)FRAME_CYCLES * SDL_NS_PER_SECOND / CLOCK_HZ;
  uint64_t next = SDL_GetTicksNS();
  int running = 1;
  while (running && cycle_counter < cycle_limit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = 0;
      } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat && !handle_key(&screen, event.key.scancode)) {
        running = 0;
      }
    }
    io_joypad(read_buttons());
    run_frame();
    play_audio(audio);
    if (screen.rslcd) {
      draw_rslcd(&screen);
    } else {
      draw_plain(&screen);
    }
    SDL_RenderPresent(screen.renderer);

    next += frame_ns;
    uint64_t now = SDL_GetTicksNS();
    if (now < next) {
      SDL_DelayPrecise(next - now);
    } else if (now - next > frame_ns * MAX_LAG_FRAMES) {
      next = now;
    }
  }

  if (audio != NULL) {
    SDL_DestroyAudioStream(audio);
  }
  if (screen.lcd != NULL) {
    SDL_DestroyTexture(screen.lcd);
  }
  SDL_DestroyTexture(screen.plain);
  SDL_DestroyRenderer(screen.renderer);
  SDL_DestroyWindow(screen.window);
  SDL_Quit();
  return 1;
}
