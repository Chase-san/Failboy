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
 
#ifndef __SONGBIRD_FILES_H__
#define __SONGBIRD_FILES_H__

/* If you don't have IO don't include this file... */
#include <stdio.h>

#ifndef __SB_NO_ALLOC__
#include <stdlib.h>
#define sb_malloc malloc
#define sb_realloc realloc
#define sb_free free
#endif /* __SB_NO_ALLOC__ */

#ifdef __cplusplus
/* Not sure why you would want to use this in C++, but just in case. */
extern "C" {
#define __songbird_header_inline__ inline
/* Works even if __STDC_VERSION__ is not defined. */
#elif __STDC_VERSION__ <= 199409L
#define __songbird_header_inline__ static __inline__
#else
#define __songbird_header_inline__ static inline
#endif

__songbird_header_inline__ unsigned sb_file_size(char const *);
__songbird_header_inline__ void *sb_file_load(char const *, unsigned const);
__songbird_header_inline__ void *sb_file_load2(char const *, unsigned *);
__songbird_header_inline__ void sb_file_write(char const *, void const *, unsigned const);

__songbird_header_inline__ unsigned sb_file_size(char const *filename) {
  FILE *f;
  unsigned size;
  /* TODO determine if STAT exists somehow. So we have the option of faster ways to do this. */
  /* This method itself isn't exactly cross platform either. But it `usually` works. */
  f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fclose(f);
  return size;
}

__songbird_header_inline__ void *sb_file_load(char const *filename, unsigned const size) {
  FILE *f;
  void *ptr;
  ptr = sb_malloc(size);
  f = fopen(filename, "rb");
  fread(ptr, size, 1, f);
  fclose(f);
  return ptr;
}

__songbird_header_inline__ void *sb_file_load2(char const *filename, unsigned *size) {
  *size = sb_file_size(filename);
  return sb_file_load(filename, *size);
}

__songbird_header_inline__ void sb_file_write(char const *filename, void const *ptr, unsigned const size) {
  FILE *f;
  f = fopen(filename, "wb");
  fwrite(ptr, size, 1, f);
  fclose(f);
}

#undef __songbird_header_inline__

#ifdef __cplusplus
}
#endif

#endif /* __SONGBIRD_FILES_H__ */
