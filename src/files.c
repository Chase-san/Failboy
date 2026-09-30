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

#include "files.h"

#include <stdio.h>
#include <stdlib.h>

void *file_load(const char *filename, unsigned int *size) {
  FILE *f;
  long length;
  void *data;

  f = fopen(filename, "rb");
  if (f == NULL) {
    return NULL;
  }
  /* Seeking to the end of a binary stream isn't guaranteed by the standard, but it `usually` works. */
  if (fseek(f, 0, SEEK_END) != 0 || (length = ftell(f)) <= 0 || fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return NULL;
  }
  data = malloc(length);
  if (data != NULL && fread(data, length, 1, f) != 1) {
    free(data);
    data = NULL;
  }
  fclose(f);
  if (data != NULL) {
    *size = (unsigned int)length;
  }
  return data;
}
