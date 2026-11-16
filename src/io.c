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

#include <string.h>

#include "core.h"

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
  SB_NO_PARTNER = 0xFF, /* the line idles high, so a lone Game Boy receives all 1s */
  SERIAL_BITS = 8,
  TAC_ENABLE = 0x04,
  TAC_CLOCK = 0x03,
  DIV_SERIAL_BIT = 0x0100,    /* the internal serial clock shifts a bit on its falling edges: 8192 Hz */
  DIV_SEQUENCER_BIT = 0x1000, /* the APU's frame sequencer steps on its falling edges: 512 Hz (0x2000 double speed) */
  DIV_RARE_BITS = DIV_SERIAL_BIT | DIV_SEQUENCER_BIT | DIV_SEQUENCER_BIT << 1, /* any of those */
  DIV_AFTER_BOOT = 0xABCC,  /* where the DMG's boot ROM leaves the divider */
  DIV_AFTER_DOUBLE = 4,     /* and where a CGB's starts again when it leaves double speed (to double, from 0) */
  TIMA_RELOAD_M_CYCLES = 1, /* M-cycles TIMA reads 00 for, past FF, before it's loaded from TMA */
  /* A CGB's boot ROM takes as long as the header makes it, so these are two games': an original Game Boy game (in */
  /* Mooneye's boot_div tests) and a color game (BullyGB's). */
  DIV_AFTER_COMPAT_BOOT = 0x2678,
  DIV_AFTER_CGB_BOOT = 0x1E30,
  IO_CGB_START = 0xFF4C, /* FF4C-FF7F: the CGB's registers, with nothing behind them on the DMG */
  OPEN_BUS = 0xFF,       /* reading nothing: the data bus floats high */
  KEY1_SWITCH = 0x01,    /* armed for STOP to switch speed */
  KEY1_DOUBLE = 0x80,
  KEY1_UNUSED = 0x7E,
  SVBK_BANK = 0x07,
  SVBK_UNUSED = 0xF8,
  IO_FF72 = 0xFF72, /* CGB: FF72-FF75 just hold what's written, to no known end: FF74 for a color game, FF75 bits 4-6 */
  IO_FF73 = 0xFF73,
  IO_FF74 = 0xFF74,
  IO_FF75 = 0xFF75,
  FF75_UNUSED = 0x8F,
};

/* FF00-FF0F: the bits that read back as 1, being unused, or in registers with nothing behind them */
static const uint8_t unused_bits[0x10] = {
    0xC0, 0x00, 0x7E, 0xFF, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0,  // 0xFF00 to 0xFF0F
};

uint8_t io_reg[IO_SIZE];
uint8_t io_ie = 0;

/* DIV is the top byte of this 16-bit counter, which ticks every T-cycle. */
static uint16_t div_counter = DIV_AFTER_BOOT;

/* TAC selects which divider bit TIMA counts the falling edges of: 4096, 262144, 65536, 16384 Hz */
static const uint8_t tac_bit[4] = {9, 3, 5, 7};

/* bits left to shift in the current serial transfer */
static uint8_t serial_bits = 0;

/* OAM DMA copies a byte each M-cycle over a bus it shares with the CPU. While it does, the CPU can't get at OAM, or */
/* at anything on that bus, where it reads DMA's byte instead; its writes to either go nowhere. A write to DMA */
/* (re)starts it after a short delay, the old transfer carrying on until then. */
enum {
  DMA_IDLE = OAM_SIZE,  /* dma_next with no transfer going */
  DMA_START_CYCLES = 2, /* M-cycles from the write until OAM is taken; the first byte comes the M-cycle after */
  OAM_BUSY = 0xFF,      /* what the CPU reads from OAM meanwhile */
  WRAM_HALF = 0x1000,   /* the address line that picks C000-CFFF or D000-DFFF */
};

/* the buses, as far as OAM DMA is concerned */
enum {
  BUS_EXTERNAL, /* the cartridge and work RAM; just the cartridge on a CGB, which gives work RAM its own */
  BUS_WRAM,     /* CGB */
  BUS_VRAM,
  BUS_OAM,
  BUS_INTERNAL, /* the registers and HRAM, which DMA never gets in the way of */
};

static uint16_t dma_source = 0;
static int dma_bus = BUS_EXTERNAL; /* the one dma_source is on */
static int dma_next = DMA_IDLE;
static uint16_t dma_restart = 0; /* the source written, starting when dma_starting runs out */
static int dma_starting = 0;
/* TIMA past FF: M-cycles until it's loaded from TMA, and whether this is the M-cycle that happened in */
static uint8_t tima_reload = 0;
static uint8_t tima_reloaded = 0;
static int dma_fought = 0;          /* the CPU drove DMA's bus this M-cycle, */
static uint8_t dma_fought_with = 0; /* with this */

/* JOYPAD_* buttons held down */
static uint8_t joypad = 0;

void io_request(uint8_t mask) { IO_REG(IO_IF) |= mask; }

void io_reset(void) {
  memset(io_reg, 0, sizeof(io_reg));
  io_ie = 0;
  div_counter = 0;
  serial_bits = 0;
  dma_source = 0;
  dma_bus = BUS_EXTERNAL;
  dma_next = DMA_IDLE;
  dma_restart = 0;
  dma_starting = 0;
  dma_fought = 0;
  tima_reload = 0;
  tima_reloaded = 0;
  joypad = 0;
}

void io_bios_init(void) {
  div_counter = DIV_AFTER_BOOT;
  if (cgb && !cgb_mode) {
    div_counter = DIV_AFTER_COMPAT_BOOT;
  } else if (cgb_mode) {
    div_counter = DIV_AFTER_CGB_BOOT;
  }
}

void io_joypad(uint8_t pressed) {
  /* a press pulls an input line low, which is what requests the interrupt, and ends STOP */
  if (pressed & ~joypad) {
    io_request(INT_JOYPAD);
    stopped = 0;
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

/* a button on a line P1 has selected is down */
int io_joypad_pressed(void) { return (joypad_read() & P1_INPUTS) != P1_INPUTS; }

/* The PPU's registers; DMA sits in the middle of them but stays here. A CGB game has more. */
static int is_video_register(uint16_t address) {
  if (address >= IO_LCDC && address <= IO_WX) {
    return address != IO_DMA;
  }
  if (!cgb_mode) {
    return 0;
  }
  return address == IO_VBK || (address >= IO_HDMA1 && address <= IO_HDMA5) ||
         (address >= IO_BCPS && address <= IO_OPRI);
}

/* sound, and the CGB's PCM12/PCM34 */
static int is_audio_register(uint16_t address) {
  if (address >= AUDIO_START && address <= AUDIO_END) {
    return 1;
  }
  return cgb && (address == IO_PCM12 || address == IO_PCM34);
}

static int timer_signal(void) {
  uint8_t tac = IO_REG(IO_TAC);
  return (tac & TAC_ENABLE) && ((div_counter >> tac_bit[tac & TAC_CLOCK]) & 1);
}

/* Call after anything that can move the timer signal (DIV ticking or reset, TAC writes). When TIMA goes past FF it */
/* reads 00 for an M-cycle; only then is it loaded from TMA and the interrupt asked for. Past FF by a write to DIV */
/* or TAC, the M-cycle of the write is that M-cycle. */
static void timer_update(int before) {
  if (before && !timer_signal()) {
    if (++IO_REG(IO_TIMA) == 0) {
      tima_reload = TIMA_RELOAD_M_CYCLES;
    }
  }
}

/* an M-cycle of the reload: counted down to it, and then for the one M-cycle it lasts */
static void timer_cycle(void) {
  tima_reloaded = 0;
  if (tima_reload && --tima_reload == 0) {
    IO_REG(IO_TIMA) = IO_REG(IO_TMA);
    io_request(INT_TIMER);
    tima_reloaded = 1;
  }
}

static void serial_shift(void) {
  if (serial_bits && --serial_bits == 0) {
    IO_REG(IO_SB) = SB_NO_PARTNER;
    IO_REG(IO_SC) &= ~SC_TRANSFER;
    if (!doctor) {
      io_request(INT_SERIAL);
    }
  }
}

/* Moves the divider, which clocks TIMA, the serial port and the APU's frame sequencer off falling edges of its bits. */
static void div_set(uint16_t value) {
  int before = timer_signal();
  uint16_t fallen = div_counter & ~value;
  div_counter = value;
  timer_update(before);
  if (!(fallen & DIV_RARE_BITS)) {
    return;
  }
  if (fallen & DIV_SERIAL_BIT) {
    serial_shift();
  }
  /* the sequencer keeps to 512 Hz in double speed */
  if (fallen & (DIV_SEQUENCER_BIT << double_speed)) {
    audio_sequencer_clock();
  }
}

/* CGB: a game has KEY1 armed for a speed switch */
int io_speed_armed(void) { return cgb_mode && (IO_REG(IO_KEY1) & KEY1_SWITCH); }

/* CGB: STOP switches speed, if KEY1 is armed for it; returns whether it did. It's late when an interrupt was */
/* waiting, and the CPU carries on through it. */
int io_speed_switch(int late) {
  if (!io_speed_armed()) {
    return 0;
  }
  double_speed = !double_speed;
  IO_REG(IO_KEY1) = 0;
  /* the divider starts again: an M-cycle of the double speed on, when that's the speed left and the CPU is */
  /* stopped for the switch */
  if (double_speed || late) {
    div_set(0);
  } else {
    div_set(DIV_AFTER_DOUBLE);
  }
  return 1;
}

static int bus_of(uint16_t address) {
  if (address >= IO_P1) {
    return BUS_INTERNAL;
  }
  if (address >= OAM_START) {
    return BUS_OAM;
  }
  if (address >= VRAM_START && address < VRAM_START + VRAM_SIZE) {
    return BUS_VRAM;
  }
  if (address >= WRAM_START && cgb) {
    return BUS_WRAM;
  }
  return BUS_EXTERNAL;
}

int io_dma_conflict(uint16_t address) {
  int bus;
  if (dma_next == DMA_IDLE) {
    return 0;
  }
  bus = bus_of(address);
  return bus == BUS_OAM || bus == dma_bus;
}

/* The byte DMA finds at its source. From E000 up a DMG's finds work RAM again, as the CPU does up to FDFF; a CGB's */
/* finds nothing there, and reads FF. */
static uint8_t dma_source_byte(void) {
  uint16_t address = dma_source + dma_next;
  if (address >= ECHO_START) {
    if (cgb) {
      return OPEN_BUS;
    }
    address -= ECHO_START - WRAM_START;
  }
  return mem_read(address);
}

/* the bus a DMA from there reads: from E000 up it's the external one, whatever the CPU finds there */
static int dma_bus_of(uint16_t source) {
  if (source >= ECHO_START) {
    return BUS_EXTERNAL;
  }
  return bus_of(source);
}

/* a CGB's VRAM bus gives DMA 00 when the CPU is on it too, reading or writing */
static void dma_fight_vram(void) {
  if (cgb && dma_bus == BUS_VRAM) {
    dma_fought = 1;
    dma_fought_with = 0;
  }
}

uint8_t io_dma_read(uint16_t address) {
  if (bus_of(address) == BUS_OAM) {
    return OAM_BUSY;
  }
  /* the byte DMA reads this M-cycle */
  uint8_t value = dma_source_byte();
  dma_fight_vram();
  return value;
}

/* The CPU writes while DMA has the bus: nothing is written where the CPU meant, and DMA reads what the CPU put on */
/* the bus in place of its source's byte; but work RAM on a DMG answers as well, and DMA gets the two ANDed. A */
/* CGB's work RAM, on a bus of its own, just gives DMA its byte. */
void io_dma_write(uint16_t address, uint8_t value) {
  if (bus_of(address) == BUS_OAM || dma_bus == BUS_WRAM) {
    return;
  }
  dma_fought = 1;
  dma_fought_with = value;
  if (!cgb && dma_source >= WRAM_START) {
    dma_fought_with &= dma_source_byte();
  }
  dma_fight_vram();
}

/* CGB: where the CPU's access to work RAM really goes while DMA reads the external bus. Work RAM's top address */
/* line follows DMA's address then, so the half the CPU gets (C000 or D000) is the one DMA's address would pick. */
uint16_t io_dma_wram(uint16_t address) {
  if (!cgb || dma_next == DMA_IDLE || dma_bus != BUS_EXTERNAL || bus_of(address) != BUS_WRAM) {
    return address;
  }
  if (address >= ECHO_START) {
    address -= ECHO_START - WRAM_START;
  }
  return (address & ~WRAM_HALF) | ((dma_source + dma_next) & WRAM_HALF);
}

/* the byte of OAM that DMA wrote in the M-cycle just run, or -1 with none going */
int io_dma_byte(void) {
  if (dma_next == 0 || dma_next == DMA_IDLE) {
    return -1;
  }
  return dma_next - 1;
}

static void dma_cycle(void) {
  if (dma_next < DMA_IDLE) {
    oam[dma_next] = dma_source_byte();
    if (dma_fought) {
      oam[dma_next] = dma_fought_with;
    }
    ++dma_next;
  }
  dma_fought = 0;
  /* a (re)start takes over at the end of the M-cycle, and copies from the next */
  if (dma_starting && --dma_starting == 0) {
    dma_source = dma_restart;
    dma_bus = dma_bus_of(dma_source);
    dma_next = 0;
  }
}

void io_tick(uint32_t cycles) {
  for (; cycles >= M_CYCLE; cycles -= M_CYCLE) {
    timer_cycle();
    div_set(div_counter + M_CYCLE);
    dma_cycle();
  }
  /* part of an M-cycle, which only the wait after a speed switch ends with: too short for OAM DMA to move */
  if (cycles) {
    div_set(div_counter + cycles);
  }
}

uint8_t io_read(uint16_t address) {
  if (is_video_register(address)) {
    return video_read(address);
  }
  if (is_audio_register(address)) {
    return audio_read(address);
  }
  switch (address) {
    case IO_P1:
      return joypad_read();
    case IO_DIV:
      return div_counter >> 8;
    case IO_IF:
      /* Gameboy Doctor's logs have the unused bits clear */
      if (doctor) {
        return IO_REG(IO_IF);
      }
      return IO_REG(IO_IF) | unused_bits[IO_IF - IO_P1];
    case IO_DMA:
      return IO_REG(IO_DMA);
    case IO_KEY1:
      if (!cgb_mode) {
        return OPEN_BUS;
      }
      return KEY1_UNUSED | (double_speed * KEY1_DOUBLE) | IO_REG(IO_KEY1);
    case IO_SVBK:
      if (!cgb_mode) {
        return OPEN_BUS;
      }
      return SVBK_UNUSED | IO_REG(IO_SVBK);
    case IO_FF72:
    case IO_FF73:
      if (!cgb) {
        return OPEN_BUS;
      }
      return IO_REG(address);
    case IO_FF74:
      if (!cgb_mode) {
        return OPEN_BUS;
      }
      return IO_REG(IO_FF74);
    case IO_FF75:
      if (!cgb) {
        return OPEN_BUS;
      }
      return FF75_UNUSED | IO_REG(IO_FF75);
    case IO_IE:
      return io_ie;
    default:
      if (address >= IO_CGB_START) {
        return OPEN_BUS;
      }
      return IO_REG(address) | unused_bits[address - IO_P1];
  }
}

void io_write(uint16_t address, uint8_t value) {
  int before;
  uint16_t source;
  if (is_video_register(address)) {
    video_write(address, value);
    return;
  }
  if (is_audio_register(address)) {
    audio_write(address, value);
    return;
  }
  switch (address) {
    case IO_SC:
      IO_REG(IO_SC) = value;
      /* the byte goes to the host, with nothing on the other end of the cable */
      if ((value & SC_TRANSFER) && (value & SC_INTERNAL_CLOCK)) {
        host_serial(IO_REG(IO_SB));
        serial_bits = SERIAL_BITS;
      }
      break;
    case IO_DIV:
      div_set(0);
      break;
    case IO_TIMA:
      /* in the M-cycle it's loaded from TMA a write is lost; in the one before, it stops the load and the interrupt */
      if (!tima_reloaded) {
        IO_REG(IO_TIMA) = value;
        tima_reload = 0;
      }
      break;
    case IO_TMA:
      IO_REG(IO_TMA) = value;
      /* TMA is still going into TIMA for the M-cycle of the load */
      if (tima_reloaded) {
        IO_REG(IO_TIMA) = value;
      }
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
      IO_REG(IO_DMA) = value;
      source = value << 8;
      dma_restart = source;
      dma_starting = DMA_START_CYCLES;
      break;
    case IO_KEY1:
      IO_REG(IO_KEY1) = value & KEY1_SWITCH;
      break;
    case IO_SVBK:
      if (cgb_mode) {
        IO_REG(IO_SVBK) = value & SVBK_BANK;
        mem_wram_bank(value & SVBK_BANK);
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
