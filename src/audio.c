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

/* The APU: two square channels (the first with a frequency sweep), a wave channel and a noise channel, clocked by */
/* the 512 Hz frame sequencer and mixed down to stereo, which the front end takes as samples at AUDIO_RATE. */

#include "failboy.h"

enum {
  NR10 = 0xFF10, /* channel 1, square with sweep: sweep */
  NR11 = 0xFF11, /* duty, length */
  NR12 = 0xFF12, /* volume envelope */
  NR13 = 0xFF13, /* frequency, low byte */
  NR14 = 0xFF14, /* trigger, length enable, frequency high bits */
  NR21 = 0xFF16, /* channel 2, square: as channel 1, without the sweep */
  NR22 = 0xFF17,
  NR23 = 0xFF18,
  NR24 = 0xFF19,
  NR30 = 0xFF1A, /* channel 3, wave: DAC power */
  NR31 = 0xFF1B, /* length */
  NR32 = 0xFF1C, /* volume */
  NR33 = 0xFF1D,
  NR34 = 0xFF1E,
  NR41 = 0xFF20,         /* channel 4, noise: length */
  NR42 = 0xFF21,         /* volume envelope */
  NR43 = 0xFF22,         /* clock shift, width, divisor */
  NR44 = 0xFF23,         /* trigger, length enable */
  NR50 = 0xFF24,         /* master volume, left and right */
  NR51 = 0xFF25,         /* which channels go left (high nibble) and right (low nibble) */
  NR52 = 0xFF26,         /* power, and which channels are on */
  WAVE_RAM = 0xFF30,     /* 32 4-bit samples, high nibble first */
  CHANNEL_REGISTERS = 5, /* NRx0 to NRx4 for channel x, from NR10 on; NR20 and NR40 are unused */
};

enum {
  NR10_PERIOD = 0x70,
  NR10_PERIOD_SHIFT = 4,
  NR10_NEGATE = 0x08,
  NR10_SHIFT = 0x07,
  DUTY_SHIFT = 6,            /* NRx1 */
  ENVELOPE_VOLUME_SHIFT = 4, /* NRx2: the starting volume */
  ENVELOPE_DAC = 0xF8,       /* a starting volume or a rising envelope; with neither, the channel's DAC is off */
  ENVELOPE_RISE = 0x08,
  ENVELOPE_PERIOD = 0x07,
  CONTROL_TRIGGER = 0x80, /* NRx4 */
  CONTROL_LENGTH = 0x40,
  CONTROL_FREQUENCY = 0x07, /* the frequency's high bits */
  NR30_DAC = 0x80,
  NR32_VOLUME = 0x60,
  NR32_VOLUME_SHIFT = 5,
  NR43_CLOCK_SHIFT = 4, /* bits 4-7: how far the divisor is shifted up */
  NR43_NARROW = 0x08,   /* a 7-bit LFSR */
  NR43_DIVISOR = 0x07,
  NR50_LEFT = 0x70,
  NR50_LEFT_SHIFT = 4,
  NR50_RIGHT = 0x07,
  PAN_LEFT = 0x10, /* NR51, channel 1's bits */
  PAN_RIGHT = 0x01,
  NR52_POWER = 0x80,
};

enum {
  CHANNELS = 4,
  SQUARE1 = 0,
  SQUARE2 = 1,
  WAVE = 2,
  NOISE = 3,
  LENGTH_FULL = 64, /* the length counter's full count */
  WAVE_LENGTH_FULL = 256,
  VOLUME_MAX = 15,
  FREQUENCIES = 2048, /* 11 bits: a channel's timer counts (2048 - frequency) times its multiplier */
  SQUARE_MULTIPLIER = 4,
  WAVE_MULTIPLIER = 2,
  SQUARE_STEPS = 8,
  WAVE_SAMPLES = 32,
  TIMER_PERIOD_ZERO = 8, /* the sweep and envelope timers count a period of 0 as 8 */
  LFSR_RESET = 0x7FFF,
  LFSR_TOP = 14,       /* the new bit goes in at the top of the 15... */
  LFSR_NARROW_TOP = 6, /* ...and in 7-bit mode, here as well */
  NOISE_STOPPED = 14,  /* clock shifts of 14 and 15 stop the noise */
  SEQUENCER_STEPS = 8,
  DAC_TOP = 15, /* a DAC's output runs from +15 at digital 0 down to -15 at digital 15 */
  LEFT = 0,
  RIGHT = 1,
  SIDES = 2,
  SAMPLE_BUFFER = 4096,                              /* sample frames held for the front end, about 86 ms */
  PENDING_MAX = SAMPLE_BUFFER * AUDIO_SAMPLE_CYCLES, /* the most T-cycles left for catch_up(): a buffer's worth */
  SAMPLE_GAIN = 32, /* the loudest mix is +-480, and the filter can at most double that: +-30720 */
};

/* what the frame sequencer clocks on each step */
enum {
  CLOCK_LENGTH = 0x01,   /* 256 Hz */
  CLOCK_SWEEP = 0x02,    /* 128 Hz */
  CLOCK_ENVELOPE = 0x04, /* 64 Hz */
};

static const uint8_t sequence[SEQUENCER_STEPS] = {
    CLOCK_LENGTH, 0, CLOCK_LENGTH | CLOCK_SWEEP, 0, CLOCK_LENGTH, 0, CLOCK_LENGTH | CLOCK_SWEEP, CLOCK_ENVELOPE,
};

/* NRx1 duty: which of the 8 steps are high, step 0 at the top */
static const uint8_t duty_waves[4] = {
    0x01, /* 12.5% */
    0x81, /* 25% */
    0x87, /* 50% */
    0x7E, /* 75% */
};

/* NR32 volume: how far the wave samples are shifted down */
static const uint8_t wave_shift[4] = {
    4, /* mute */
    0, /* 100% */
    1, /* 50% */
    2, /* 25% */
};

/* NR43 divisor: T-cycles between noise clocks, before the clock shift */
static const uint8_t noise_divisor[8] = {
    8, 16, 32, 48, 64, 80, 96, 112,
};

/* bits that read back as 1: the write-only and unused ones */
static const uint8_t read_mask[0x20] = {
    0x80, 0x3F, 0x00, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF,  // 0xFF10 to 0xFF1F
    0xFF, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x70, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 0xFF20 to 0xFF2F
};

/* The DMG's output capacitor charges by 0.999958 a T-cycle; this is that over a sample's 88. */
static const float CAPACITOR_CHARGE = 0.99631f;

struct channel {
  int on;       /* NR52's status bit */
  int length;   /* length clocks left before the channel stops, when NRx4 enables that */
  int volume;   /* 0 to 15, from the envelope */
  int envelope; /* envelope clocks until its next step */
  int timer;    /* T-cycles until the next step of the waveform */
  int position; /* the step of the duty cycle, or of the wave samples */
};

static struct channel channels[CHANNELS];
static int sequencer; /* the frame sequencer's next step */
static int sweep_on;
static int sweep_timer;     /* sweep clocks until the next sweep */
static int sweep_frequency; /* the sweep's own copy of channel 1's frequency */
static int sweep_negated;   /* the sweep has subtracted since channel 1 was triggered */
static uint8_t wave_sample; /* the wave channel's current 4-bit sample */
static uint16_t lfsr = LFSR_RESET;

static int pending;             /* T-cycles the channels haven't been run for yet */
static int levels[SIDES];       /* the mix as it stands */
static int levels_stale = 1;    /* something it depends on has changed since */
static int sums[SIDES];         /* the mix, added up over each T-cycle of the sample so far */
static int sample_clock;        /* T-cycles into the sample */
static float capacitors[SIDES]; /* the output's high-pass filter */
static int16_t samples[SAMPLE_BUFFER * SIDES];
static unsigned int sample_frames;

/* NRxn of channel ch */
static uint8_t reg(int ch, int n) { return IO_REG(NR10 + ch * CHANNEL_REGISTERS + n); }

static int frequency(int ch) { return reg(ch, 3) | ((reg(ch, 4) & CONTROL_FREQUENCY) << 8); }

/* T-cycles per step of the channel's waveform */
static int period(int ch) {
  switch (ch) {
    case WAVE:
      return (FREQUENCIES - frequency(ch)) * WAVE_MULTIPLIER;
    case NOISE:
      return noise_divisor[reg(ch, 3) & NR43_DIVISOR] << (reg(ch, 3) >> NR43_CLOCK_SHIFT);
    default:
      return (FREQUENCIES - frequency(ch)) * SQUARE_MULTIPLIER;
  }
}

static int dac_on(int ch) {
  if (ch == WAVE) {
    return IO_REG(NR30) & NR30_DAC;
  }
  return reg(ch, 2) & ENVELOPE_DAC;
}

static int full_length(int ch) {
  if (ch == WAVE) {
    return WAVE_LENGTH_FULL;
  }
  return LENGTH_FULL;
}

/* NRx1: the length counter counts up from the value written; this keeps what's left to go */
static void load_length(int ch, uint8_t value) {
  int full = full_length(ch);
  channels[ch].length = full - (value & (full - 1));
}

static int timer_period(int period) {
  if (period == 0) {
    return TIMER_PERIOD_ZERO;
  }
  return period;
}

/* The sweep's next frequency; past the top, channel 1 stops. */
static int sweep_next(void) {
  uint8_t nr10 = IO_REG(NR10);
  int delta = sweep_frequency >> (nr10 & NR10_SHIFT);
  if (nr10 & NR10_NEGATE) {
    sweep_negated = 1;
    return sweep_frequency - delta;
  }
  if (sweep_frequency + delta >= FREQUENCIES) {
    channels[SQUARE1].on = 0;
  }
  return sweep_frequency + delta;
}

static void trigger_sweep(void) {
  uint8_t nr10 = IO_REG(NR10);
  sweep_frequency = frequency(SQUARE1);
  sweep_timer = timer_period((nr10 & NR10_PERIOD) >> NR10_PERIOD_SHIFT);
  sweep_on = (nr10 & (NR10_PERIOD | NR10_SHIFT)) != 0;
  sweep_negated = 0;
  if (nr10 & NR10_SHIFT) {
    sweep_next(); /* only to check for overflow */
  }
}

static void clock_sweep(void) {
  uint8_t nr10 = IO_REG(NR10);
  int sweep_period = (nr10 & NR10_PERIOD) >> NR10_PERIOD_SHIFT;
  if (--sweep_timer > 0) {
    return;
  }
  sweep_timer = timer_period(sweep_period);
  if (!sweep_on || sweep_period == 0) {
    return;
  }
  int next = sweep_next();
  if (next < FREQUENCIES && (nr10 & NR10_SHIFT)) {
    sweep_frequency = next;
    IO_REG(NR13) = next & 0xFF;
    IO_REG(NR14) = (IO_REG(NR14) & ~CONTROL_FREQUENCY) | (next >> 8);
    sweep_next(); /* again with the new frequency, only to check for overflow */
  }
}

static void clock_length(int ch) {
  struct channel *c = &channels[ch];
  if ((reg(ch, 4) & CONTROL_LENGTH) && c->length > 0) {
    --c->length;
    if (c->length == 0) {
      c->on = 0;
    }
  }
}

static void clock_envelope(int ch) {
  struct channel *c = &channels[ch];
  uint8_t nrx2 = reg(ch, 2);
  int envelope_period = nrx2 & ENVELOPE_PERIOD;
  if (envelope_period == 0) {
    return;
  }
  if (--c->envelope > 0) {
    return;
  }
  c->envelope = envelope_period;
  if ((nrx2 & ENVELOPE_RISE) && c->volume < VOLUME_MAX) {
    ++c->volume;
  } else if (!(nrx2 & ENVELOPE_RISE) && c->volume > 0) {
    --c->volume;
  }
}

/* NRx4 bit 7 restarts the channel; extra_clock is write_control()'s. */
static void trigger(int ch, int extra_clock) {
  struct channel *c = &channels[ch];
  c->on = dac_on(ch) != 0; /* with its DAC off it stops again right away */
  if (c->length == 0) {
    c->length = full_length(ch);
    if (extra_clock && (reg(ch, 4) & CONTROL_LENGTH)) {
      --c->length;
    }
  }
  c->timer = period(ch);
  if (ch != WAVE) {
    c->volume = reg(ch, 2) >> ENVELOPE_VOLUME_SHIFT;
    c->envelope = timer_period(reg(ch, 2) & ENVELOPE_PERIOD);
  }
  switch (ch) {
    case SQUARE1:
      trigger_sweep();
      break;
    case WAVE:
      c->position = 0; /* the sample playing stays until the first step */
      break;
    case NOISE:
      lfsr = LFSR_RESET;
      break;
    default:
      break;
  }
}

/* NRx4. When the frame sequencer's next step doesn't clock lengths, turning the length counter on clocks it once */
/* right away, and a trigger that has to reload it (with it on) loads one less. */
static void write_control(int ch, uint8_t old, uint8_t value) {
  struct channel *c = &channels[ch];
  int extra_clock = !(sequence[sequencer] & CLOCK_LENGTH);
  if (extra_clock && !(old & CONTROL_LENGTH) && (value & CONTROL_LENGTH) && c->length > 0) {
    --c->length;
    if (c->length == 0 && !(value & CONTROL_TRIGGER)) {
      c->on = 0;
    }
  }
  if (value & CONTROL_TRIGGER) {
    trigger(ch, extra_clock);
  }
}

/* NR52 bit 7. Off, every register but NR52 and wave RAM clears and ignores writes until it's back on, though the DMG */
/* keeps its length counters. On, the frame sequencer and the waveforms start over. */
static void set_power(int on) {
  if (!on) {
    for (uint16_t address = NR10; address < NR52; ++address) {
      IO_REG(address) = 0;
    }
    for (int ch = 0; ch < CHANNELS; ++ch) {
      channels[ch].on = 0;
    }
    IO_REG(NR52) = 0;
    return;
  }
  if (!(IO_REG(NR52) & NR52_POWER)) {
    IO_REG(NR52) = NR52_POWER;
    sequencer = 0;
    channels[SQUARE1].position = 0;
    channels[SQUARE2].position = 0;
    wave_sample = 0;
  }
}

/* The channel's digital output, 0 to 15. */
static int output(int ch) {
  const struct channel *c = &channels[ch];
  if (!c->on) {
    return 0;
  }
  switch (ch) {
    case WAVE:
      return wave_sample >> wave_shift[(IO_REG(NR32) & NR32_VOLUME) >> NR32_VOLUME_SHIFT];
    case NOISE:
      return (~lfsr & 1) * c->volume;
    default:
      return ((duty_waves[reg(ch, 1) >> DUTY_SHIFT] >> (SQUARE_STEPS - 1 - c->position)) & 1) * c->volume;
  }
}

/* The channel's timer ran out: on to the next step of its waveform. */
static void advance(int ch) {
  struct channel *c = &channels[ch];
  switch (ch) {
    case WAVE: {
      c->position = (c->position + 1) % WAVE_SAMPLES;
      uint8_t pair = IO_REG(WAVE_RAM + c->position / 2);
      if (c->position & 1) {
        wave_sample = pair & 0x0F;
      } else {
        wave_sample = pair >> 4;
      }
      break;
    }
    case NOISE: {
      uint8_t nr43 = reg(ch, 3);
      if ((nr43 >> NR43_CLOCK_SHIFT) >= NOISE_STOPPED) {
        break;
      }
      int bit = (lfsr ^ (lfsr >> 1)) & 1;
      lfsr = (lfsr >> 1) | (bit << LFSR_TOP);
      if (nr43 & NR43_NARROW) {
        lfsr = (lfsr & ~(1 << LFSR_NARROW_TOP)) | (bit << LFSR_NARROW_TOP);
      }
      break;
    }
    default:
      c->position = (c->position + 1) % SQUARE_STEPS;
      break;
  }
}

/* The channels' DACs, panned by NR51 and scaled by NR50's volumes (1 to 8). */
static void mix(void) {
  uint8_t panning = IO_REG(NR51);
  uint8_t master = IO_REG(NR50);
  int left = 0;
  int right = 0;
  for (int ch = 0; ch < CHANNELS; ++ch) {
    if (!dac_on(ch)) {
      continue;
    }
    int dac = DAC_TOP - 2 * output(ch);
    if (panning & (PAN_LEFT << ch)) {
      left += dac;
    }
    if (panning & (PAN_RIGHT << ch)) {
      right += dac;
    }
  }
  levels[LEFT] = left * (((master & NR50_LEFT) >> NR50_LEFT_SHIFT) + 1);
  levels[RIGHT] = right * ((master & NR50_RIGHT) + 1);
  levels_stale = 0;
}

/* A finished sample: the mix averaged over its T-cycles, through the output capacitor, which takes the DC out. */
static void emit(void) {
  for (int side = 0; side < SIDES; ++side) {
    float in = (float)sums[side] / AUDIO_SAMPLE_CYCLES;
    float out = in - capacitors[side];
    capacitors[side] = in - out * CAPACITOR_CHARGE;
    samples[sample_frames * SIDES + side] = (int16_t)(out * SAMPLE_GAIN);
    sums[side] = 0;
  }
  ++sample_frames;
}

/* The channels run lazily: audio_tick() only counts the T-cycles, and this brings them up to date before anything */
/* changes them and before the samples are taken, going straight from one step of a channel (or end of a sample) to */
/* the next. */
static void catch_up(void) {
  while (pending > 0) {
    if (sample_frames == SAMPLE_BUFFER) {
      pending = 0; /* see audio_tick() */
      return;
    }
    int run = AUDIO_SAMPLE_CYCLES - sample_clock;
    if (run > pending) {
      run = pending;
    }
    for (int ch = 0; ch < CHANNELS; ++ch) {
      if (channels[ch].on && channels[ch].timer < run) {
        run = channels[ch].timer;
      }
    }
    if (levels_stale) {
      mix();
    }
    for (int side = 0; side < SIDES; ++side) {
      sums[side] += levels[side] * run;
    }
    pending -= run;
    sample_clock += run;

    for (int ch = 0; ch < CHANNELS; ++ch) {
      struct channel *c = &channels[ch];
      if (!c->on) {
        continue;
      }
      c->timer -= run;
      if (c->timer == 0) {
        c->timer = period(ch);
        advance(ch);
        levels_stale = 1;
      }
    }
    if (sample_clock == AUDIO_SAMPLE_CYCLES) {
      sample_clock = 0;
      emit();
    }
  }
}

void audio_tick(uint32_t cycles) {
  /* The channels are only heard through the samples, so with nobody taking them (headless) they needn't run. */
  if (sample_frames == SAMPLE_BUFFER) {
    return;
  }
  pending += (int)cycles;
  if (pending >= PENDING_MAX) {
    catch_up();
  }
}

void audio_sequencer_clock(void) {
  if (!(IO_REG(NR52) & NR52_POWER)) {
    return;
  }
  catch_up();
  levels_stale = 1;
  uint8_t clocks = sequence[sequencer];
  sequencer = (sequencer + 1) % SEQUENCER_STEPS;
  for (int ch = 0; ch < CHANNELS; ++ch) {
    if (clocks & CLOCK_LENGTH) {
      clock_length(ch);
    }
    if ((clocks & CLOCK_ENVELOPE) && ch != WAVE) {
      clock_envelope(ch);
    }
  }
  if (clocks & CLOCK_SWEEP) {
    clock_sweep();
  }
}

uint8_t audio_read(uint16_t address) {
  if (address >= WAVE_RAM) {
    return IO_REG(address);
  }
  uint8_t value = IO_REG(address) | read_mask[address - NR10];
  if (address == NR52) {
    for (int ch = 0; ch < CHANNELS; ++ch) {
      value |= channels[ch].on << ch;
    }
  }
  return value;
}

void audio_write(uint16_t address, uint8_t value) {
  int ch = (address - NR10) / CHANNEL_REGISTERS;
  catch_up();
  levels_stale = 1;
  if (address >= WAVE_RAM) {
    IO_REG(address) = value;
    return;
  }
  if (address > NR52) {
    return; /* nothing at FF27-FF2F */
  }
  if (address == NR52) {
    set_power(value & NR52_POWER);
    return;
  }
  if (!(IO_REG(NR52) & NR52_POWER)) {
    if (address == NR11 || address == NR21 || address == NR31 || address == NR41) {
      load_length(ch, value);
    }
    return;
  }

  uint8_t old = IO_REG(address);
  IO_REG(address) = value;
  switch (address) {
    case NR10:
      /* leaving negate mode once the sweep has subtracted stops channel 1 */
      if (sweep_negated && !(value & NR10_NEGATE)) {
        channels[SQUARE1].on = 0;
      }
      break;
    case NR11:
    case NR21:
    case NR31:
    case NR41:
      load_length(ch, value);
      break;
    case NR12:
    case NR22:
    case NR30:
    case NR42:
      /* turning a DAC off stops its channel */
      if (!dac_on(ch)) {
        channels[ch].on = 0;
      }
      break;
    case NR14:
    case NR24:
    case NR34:
    case NR44:
      write_control(ch, old, value);
      break;
    default:
      break;
  }
}

const int16_t *audio_samples(unsigned int *frames) {
  catch_up();
  *frames = sample_frames;
  sample_frames = 0;
  return samples;
}

/* How the boot ROM leaves the APU: on, with channel 1 still going after the startup chime, faded out. */
void audio_bios_init(void) {
  set_power(1);
  IO_REG(NR11) = 0x80; /* 50% duty */
  IO_REG(NR12) = 0xF3; /* volume 15, falling every 3 envelope clocks */
  IO_REG(NR13) = 0xC1; /* the chime's second note */
  IO_REG(NR14) = 0x07;
  IO_REG(NR50) = 0x77;
  IO_REG(NR51) = 0xF3;
  channels[SQUARE1].on = 1;
  channels[SQUARE1].timer = period(SQUARE1);
}
