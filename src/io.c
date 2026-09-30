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

#include <stdio.h>
#include <string.h>

#include "failboy.h"

enum {
  P1_DIRECTIONS = 0x10, /* clear to read the D-pad */
  P1_BUTTONS = 0x20,    /* clear to read A, B, Select and Start */
  P1_SELECT = 0x30,
  P1_INPUTS = 0x0F, /* active low */
  P1_UNUSED = 0xC0,
  JOYPAD_DIRECTIONS = 0x0F,
  JOYPAD_BUTTONS_SHIFT = 4,
  SC_TRANSFER = 0x80, /* start / in progress */
  SC_INTERNAL_CLOCK = 0x01,
  SC_UNUSED = 0x7E,
  SB_NO_PARTNER = 0xFF, /* the line idles high, so a lone Game Boy receives all 1s */
  SERIAL_CYCLES = 4096, /* 8 bits at 8192 Hz */
  TAC_ENABLE = 0x04,
  TAC_CLOCK = 0x03,
  TAC_UNUSED = 0xF8,
  IF_UNUSED = 0xE0,
};

uint8_t io_reg[IO_SIZE];
uint8_t io_ie = 0;

/* DIV is the top byte of this 16-bit counter, which ticks every T-cycle. */
static uint16_t div_counter = 0;

/* TAC selects which divider bit TIMA counts the falling edges of: 4096, 262144, 65536, 16384 Hz */
static const uint8_t tac_bit[4] = {9, 3, 5, 7};

/* T-cycles left in the current serial transfer */
static uint16_t serial_timer = 0;

/* The last few characters sent over serial, to spot Blargg's "Passed"/"Failed". */
static char serial_tail[6];
static int serial_result = SERIAL_NONE;

/* JOYPAD_* buttons held down */
static uint8_t joypad = 0;

void io_request(uint8_t mask) { IO_REG(IO_IF) |= mask; }

void io_joypad(uint8_t pressed) {
  /* a press pulls an input line low, which is what requests the interrupt */
  if (pressed & ~joypad) {
    io_request(INT_JOYPAD);
  }
  joypad = pressed;
}

static uint8_t joypad_read(void) {
  uint8_t select = IO_REG(IO_P1) & P1_SELECT;
  uint8_t pressed = 0;
  if (!(select & P1_DIRECTIONS)) {
    pressed |= joypad & JOYPAD_DIRECTIONS;
  }
  if (!(select & P1_BUTTONS)) {
    pressed |= joypad >> JOYPAD_BUTTONS_SHIFT;
  }
  return P1_UNUSED | select | (~pressed & P1_INPUTS);
}

/* The PPU's registers; DMA sits in the middle of them but stays here. */
static int is_video_register(uint16_t address) { return address >= IO_LCDC && address <= IO_WX && address != IO_DMA; }

static int timer_signal(void) {
  uint8_t tac = IO_REG(IO_TAC);
  return (tac & TAC_ENABLE) && ((div_counter >> tac_bit[tac & TAC_CLOCK]) & 1);
}

/* Call after anything that can move the timer signal (DIV ticking or reset, TAC writes). */
static void timer_update(int before) {
  if (before && !timer_signal()) {
    if (++IO_REG(IO_TIMA) == 0) {
      IO_REG(IO_TIMA) = IO_REG(IO_TMA);
      io_request(INT_TIMER);
    }
  }
}

void io_tick(uint32_t cycles) {
  for (; cycles >= M_CYCLE; cycles -= M_CYCLE) {
    int before = timer_signal();
    div_counter += M_CYCLE;
    timer_update(before);

    if (serial_timer) {
      serial_timer -= M_CYCLE;
      if (!serial_timer) {
        IO_REG(IO_SB) = SB_NO_PARTNER;
        IO_REG(IO_SC) &= ~SC_TRANSFER;
        if (!doctor) {
          io_request(INT_SERIAL);
        }
      }
    }
  }
}

static void serial_out(uint8_t c) {
  FILE *out = stdout;
  if (doctor) {
    /* stdout is carrying the trace */
    out = stderr;
  }
  fputc(c, out);
  fflush(out);

  memmove(serial_tail, serial_tail + 1, sizeof(serial_tail) - 1);
  serial_tail[sizeof(serial_tail) - 1] = c;
  if (memcmp(serial_tail, "Failed", sizeof(serial_tail)) == 0) {
    serial_result = SERIAL_FAILED;
  } else if (serial_result == SERIAL_NONE && memcmp(serial_tail, "Passed", sizeof(serial_tail)) == 0) {
    serial_result = SERIAL_PASSED;
  }
}

int io_serial_result(void) { return serial_result; }

uint8_t io_read(uint16_t address) {
  if (is_video_register(address)) {
    return video_read(address);
  }
  switch (address) {
    case IO_P1:
      return joypad_read();
    case IO_SC:
      return IO_REG(IO_SC) | SC_UNUSED;
    case IO_DIV:
      return div_counter >> 8;
    case IO_TAC:
      return IO_REG(IO_TAC) | TAC_UNUSED;
    case IO_IF:
      /* the unused bits read as 1, but Gameboy Doctor's logs have them clear */
      if (doctor) {
        return IO_REG(IO_IF);
      }
      return IO_REG(IO_IF) | IF_UNUSED;
    case IO_IE:
      return io_ie;
    default:
      return IO_REG(address);
  }
}

void io_write(uint16_t address, uint8_t value) {
  int before;
  if (is_video_register(address)) {
    video_write(address, value);
    return;
  }
  switch (address) {
    case IO_SC:
      IO_REG(IO_SC) = value;
      /* link cable for console ! :D */
      if ((value & SC_TRANSFER) && (value & SC_INTERNAL_CLOCK)) {
        serial_out(IO_REG(IO_SB));
        serial_timer = SERIAL_CYCLES;
      }
      break;
    case IO_DIV:
      before = timer_signal();
      div_counter = 0;
      timer_update(before);
      break;
    case IO_TAC:
      before = timer_signal();
      IO_REG(IO_TAC) = value;
      timer_update(before);
      break;
    case IO_IF:
      IO_REG(IO_IF) = value & INT_ALL;
      break;
    case IO_DMA:
      /* OAM DMA, done all at once */
      IO_REG(IO_DMA) = value;
      for (uint16_t i = 0; i < OAM_SIZE; ++i) {
        oam[i] = mem_read((value << 8) | i);
      }
      break;
    case IO_IE:
      io_ie = value;
      break;
    default:
      IO_REG(address) = value;
      break;
  }
}
