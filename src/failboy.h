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

#ifndef _FAILBOY_H_
#define _FAILBOY_H_

#include <stdint.h>

#define HIBYTE(a) ((a) >> 8)
#define LOBYTE(a) ((a) & 0xff)

typedef uint8_t (*read_f)(uint16_t);
typedef void (*write_f)(uint16_t, uint8_t);

/* cart.c */
void cart_load(const char *);
void cart_free(void);

/* mem.c */
void mem_alloc(void);
void mem_free(void);

extern uint8_t *oam;
extern uint8_t *vram;

uint8_t read(uint16_t);
uint16_t read16(uint16_t);

void write(uint16_t, uint8_t);
void write16(uint16_t, uint16_t);

/* cpu.c */
struct registers {
  uint16_t PC;

  union {
    uint16_t SP;

    struct {
      uint8_t SPLO;
      uint8_t SPHI;
    };
  };

  union {
    uint16_t AF;

    struct {
      union {
        uint8_t F;

        struct {
          uint8_t : 4;     /* 0-3 unused */
          uint8_t F_C : 1; /* carry */
          uint8_t F_H : 1; /* half-carry (BCD) */
          uint8_t F_N : 1; /* add/sub flag (BCD) */
          uint8_t F_Z : 1; /* zero flag */
        };
      };

      uint8_t A;
    };
  };

  union {
    uint16_t BC;

    struct {
      uint8_t C;
      uint8_t B;
    };
  };

  union {
    uint16_t DE;

    struct {
      uint8_t E;
      uint8_t D;
    };
  };

  union {
    uint16_t HL;

    struct {
      uint8_t L;
      uint8_t H;
    };
  };
};

extern struct registers r;
extern uint32_t cycle_counter;

#define rpc8() (read(r.PC++))
#define rpc16() ((rpc8()) | (rpc8() << 8))

void cpu_bios_init(void);
void step(void);

#endif /* _FAILBOY_H_ */
