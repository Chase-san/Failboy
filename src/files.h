/**
 *  Copyright (c) 2014, Robert Maupin <chasesan@gmail.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
#define __songbird_header_inline__	inline
/* Works even if __STDC_VERSION__ is not defined. */
#elif __STDC_VERSION__ <= 199409L
#define __songbird_header_inline__	static __inline__
#else
#define __songbird_header_inline__	static inline
#endif

__songbird_header_inline__	unsigned sb_file_size(char const *);
__songbird_header_inline__	void *sb_file_load(char const *, unsigned const);
__songbird_header_inline__	void *sb_file_load2(char const *, unsigned *);
__songbird_header_inline__	void sb_file_write(char const *, void const *, unsigned const);

__songbird_header_inline__
unsigned sb_file_size(char const *filename) {
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

__songbird_header_inline__
void *sb_file_load(char const *filename, unsigned const size) {
	FILE *f;
	void *ptr;
	ptr = sb_malloc(size);
	f = fopen(filename, "rb");
	fread(ptr, size, 1, f);
	fclose(f);
	return ptr;
}

__songbird_header_inline__
void *sb_file_load2(char const *filename, unsigned *size) {
	*size = sb_file_size(filename);
	return sb_file_load(filename, *size);
}

__songbird_header_inline__
void sb_file_write(char const *filename, void const *ptr, unsigned const size) {
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
