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

#include "sdl/rslcd.h"

#include "failboy.h"

enum {
  SHADES = 4,
  GRID_MIN_SCALE = 3, /* below this a one-pixel gap between pixels would swallow them */
};

struct color {
  float r, g, b;
};

/* How dark each shade gets, at the contrast wheel's least bad setting: still washed out at the light end and */
/* muddy at the dark end, and even fully on a pixel isn't black. */
static const float shade_ink[SHADES] = {0.05f, 0.25f, 0.64f, 0.86f};

/* Pixels are slow: each frame they only get this far towards their new darkness, and they fade slower than they */
/* darken, so anything that moves leaves a trail. */
static const float RISE_RATE = 0.45f;
static const float FALL_RATE = 0.3f;

/* Each pixel casts a shadow onto the reflector behind the glass: a copy of it moved down and to the right (by up to */
/* half a Game Boy pixel), with edges this soft (1 is as soft as it gets). */
static const float SHADOW_OFFSET_X = 0.35f;
static const float SHADOW_OFFSET_Y = 0.5f;
static const float SHADOW_SOFTNESS = 0.45f;
static const float SHADOW_STRENGTH = 0.6f;

/* The reflector is lit from the top left: a little brighter there, and dimmer towards the edges. */
static const float LIGHT_GRADIENT = 0.1f;
static const float LIGHT_VIGNETTE = 0.1f;

/* A faint sheen where the light glances off the glass: a diagonal band this far along from the top left corner. */
static const float SHEEN_CENTER = 0.3f;
static const float SHEEN_WIDTH = 0.22f;

static const struct color REFLECTOR = {176, 186, 112};
static const struct color INK = {38, 54, 44};
static const struct color SHEEN = {14, 15, 12}; /* added at the band's middle */

/* how dark each pixel is right now, 0 to 1; the border of zeros spares drawing the shadows any bounds checks */
static float ink[LCD_HEIGHT + 2][LCD_WIDTH + 2];

/* Everything that only depends on the scale, worked out once per scale. */
static int prepared_scale = 0;

/* Output pixel s of a Game Boy pixel takes its shadow from that pixel and the one s0 (-1 or 0) before it, blended by */
/* w. */
static int sub_x0[RSLCD_MAX_SCALE];
static int sub_y0[RSLCD_MAX_SCALE];
static float sub_wx[RSLCD_MAX_SCALE];
static float sub_wy[RSLCD_MAX_SCALE];

static float column_light[LCD_WIDTH * RSLCD_MAX_SCALE];
static float row_light[LCD_HEIGHT * RSLCD_MAX_SCALE];
static float sheen[(LCD_WIDTH + LCD_HEIGHT) * RSLCD_MAX_SCALE]; /* by x + y */

/* The shadow edge between two neighboring pixels: the weight of the second, at fraction f of the way from the */
/* first's center to the second's. */
static float shadow_edge(float f) {
  float w = (f - 0.5f) / SHADOW_SOFTNESS + 0.5f;
  if (w < 0) {
    return 0;
  }
  if (w > 1) {
    return 1;
  }
  return w;
}

static void sub_pixel(int scale, int s, float offset, int *s0, float *w) {
  float u = (s + 0.5f) / scale - 0.5f - offset; /* between -1 and 0.5 */
  *s0 = 0;
  if (u < 0) {
    *s0 = -1;
  }
  *w = shadow_edge(u - *s0);
}

/* light across the reflector along one axis, t from 0 (top or left) to 1 */
static float light_along(float t) {
  float edge = 2 * t - 1;
  return (1 - LIGHT_VIGNETTE * edge * edge) * (1 + LIGHT_GRADIENT * (0.5f - t));
}

static void prepare(int scale) {
  int width = LCD_WIDTH * scale;
  int height = LCD_HEIGHT * scale;
  for (int s = 0; s < scale; ++s) {
    sub_pixel(scale, s, SHADOW_OFFSET_X, &sub_x0[s], &sub_wx[s]);
    sub_pixel(scale, s, SHADOW_OFFSET_Y, &sub_y0[s], &sub_wy[s]);
  }
  for (int x = 0; x < width; ++x) {
    column_light[x] = light_along((x + 0.5f) / width);
  }
  for (int y = 0; y < height; ++y) {
    row_light[y] = light_along((y + 0.5f) / height);
  }
  for (int d = 0; d < width + height - 1; ++d) {
    float t = ((float)d / (width + height - 2) - SHEEN_CENTER) / SHEEN_WIDTH;
    sheen[d] = 0;
    if (t > -1 && t < 1) {
      sheen[d] = (1 - t * t) * (1 - t * t);
    }
  }
  prepared_scale = scale;
}

void rslcd_reset(const uint8_t *frame) {
  for (int y = 0; y < LCD_HEIGHT; ++y) {
    for (int x = 0; x < LCD_WIDTH; ++x) {
      ink[y + 1][x + 1] = shade_ink[frame[y * LCD_WIDTH + x]];
    }
  }
}

void rslcd_frame(const uint8_t *frame) {
  for (int y = 0; y < LCD_HEIGHT; ++y) {
    for (int x = 0; x < LCD_WIDTH; ++x) {
      float *dark = &ink[y + 1][x + 1];
      float target = shade_ink[frame[y * LCD_WIDTH + x]];
      float rate = FALL_RATE;
      if (target > *dark) {
        rate = RISE_RATE;
      }
      *dark += (target - *dark) * rate;
    }
  }
}

static uint32_t pack(float r, float g, float b) {
  return (uint32_t)(r + 0.5f) << 16 | (uint32_t)(g + 0.5f) << 8 | (uint32_t)(b + 0.5f);
}

void rslcd_draw(int scale, uint32_t *pixels, int pitch) {
  if (scale < 1 || scale > RSLCD_MAX_SCALE) {
    return;
  }
  if (scale != prepared_scale) {
    prepare(scale);
  }
  int grid = scale >= GRID_MIN_SCALE;

  for (int row = 0; row < LCD_HEIGHT; ++row) {
    /* for each pixel in the row: how much reflected light gets through it, and the color its ink adds */
    float keep[LCD_WIDTH];
    struct color added[LCD_WIDTH];
    for (int column = 0; column < LCD_WIDTH; ++column) {
      float dark = ink[row + 1][column + 1];
      keep[column] = 1 - dark;
      added[column].r = INK.r * dark;
      added[column].g = INK.g * dark;
      added[column].b = INK.b * dark;
    }

    for (int sy = 0; sy < scale; ++sy) {
      int y = row * scale + sy;
      /* the light left over after the shadows, blended down to this line of output (with the border, index + 1) */
      float lit[LCD_WIDTH + 2];
      const float *upper = ink[row + sub_y0[sy] + 1];
      const float *lower = ink[row + sub_y0[sy] + 2];
      for (int i = 0; i < LCD_WIDTH + 2; ++i) {
        lit[i] = 1 - (upper[i] + (lower[i] - upper[i]) * sub_wy[sy]) * SHADOW_STRENGTH;
      }
      struct color base = {REFLECTOR.r * row_light[y], REFLECTOR.g * row_light[y], REFLECTOR.b * row_light[y]};
      int gap_row = grid && sy == scale - 1;
      uint32_t *out = pixels + y * pitch;

      int x = 0;
      for (int column = 0; column < LCD_WIDTH; ++column) {
        for (int sx = 0; sx < scale; ++sx, ++x) {
          int i = column + sub_x0[sx] + 1;
          float light = (lit[i] + (lit[i + 1] - lit[i]) * sub_wx[sx]) * column_light[x];
          float r = base.r * light;
          float g = base.g * light;
          float b = base.b * light;
          /* the gaps between pixels have no liquid crystal in them, just the reflector */
          if (!gap_row && !(grid && sx == scale - 1)) {
            r = r * keep[column] + added[column].r;
            g = g * keep[column] + added[column].g;
            b = b * keep[column] + added[column].b;
          }
          /* the sheen is on the glass, in front of everything */
          float glare = sheen[x + y];
          *out++ = pack(r + SHEEN.r * glare, g + SHEEN.g * glare, b + SHEEN.b * glare);
        }
      }
    }
  }
}
