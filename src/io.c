/**
 * Copyright 2014-2026 Robert Maupin <chasesan@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
 * to permit persons to whom the Software is furnished to do so.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>

enum {
  IO_P1 = 0xFF00,
  IO_SB = 0xFF01,
  IO_SC = 0xFF02,
  IO_DIV = 0xFF04,
  IO_TIMA = 0xFF05,
  IO_TMA = 0xFF06,
  IO_TAC = 0xFF07,
  IO_IF = 0xFF0F,
  IO_DMA = 0xFF46,
  IO_IE = 0xFFFF
};

static uint8_t sb = 0;

uint8_t io_read(uint16_t address) {
  return 0;
}

void io_write(uint16_t address, uint8_t value) {
  switch (address) {
    /* link cable for console ! :D */
    case IO_SB:
      sb = value;
      break;
    case IO_SC:
      /* transfer the data */
      if (value == 0x81) {
        printf("%c", sb);
      }
      break;
    default:
      // printf("WRITE %02x -> %04x\n", value, address);
      break;
  }
}
