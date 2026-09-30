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

#ifndef _RSLCD_H_
#define _RSLCD_H_

#include <stdint.h>

/* RSLCD, the "really shitty LCD": the original Game Boy's reflective screen. Low contrast, pixels that smear, a */
/* visible grid between them, and shadows on the reflector behind the glass. */

enum {
  RSLCD_MAX_SCALE = 32,
};

/* Makes the screen show frame (LCD_WIDTH * LCD_HEIGHT shades) straight away, with no smearing from before. */
void rslcd_reset(const uint8_t *frame);

/* Moves the screen one frame's worth towards showing frame. */
void rslcd_frame(const uint8_t *frame);

/* Draws the screen as XRGB8888 at scale (1 to RSLCD_MAX_SCALE) output pixels per Game Boy pixel; pitch is in */
/* pixels. */
void rslcd_draw(int scale, uint32_t *pixels, int pitch);

#endif /* _RSLCD_H_ */
