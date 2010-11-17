/*
 * Copyright (C) 2010 Simon Kagstrom
 *
 * See COPYING for license details
 */
#ifndef __UTILS_H__
#define __UTILS_H__

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

#define BUG_ON(cond)

#define error(x...) do \
  { \
    fprintf(stderr, "=============ERROR ERROR ERROR===========\n"); \
    fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); fprintf(stderr, x); \
    fprintf(stderr, "=========================================\n"); \
    assert(0); \
    exit(1); \
  } while(0)

#define warning(x...) do \
  { \
    fprintf(stderr, "==============WARNING WARNING============\n"); \
    fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); fprintf(stderr, x); \
    fprintf(stderr, "=========================================\n"); \
  } while(0)

#define panic(x...) do { error(x); abort(); } while(0)

#define panic_if(cond, x...) \
  do { if ((cond)) panic(x); } while(0)

#define warning_if(cond, x...) \
  do { if ((cond)) warning(x); } while(0)

static inline char *xstrdup(const char *s)
{
  char *out = strdup(s);

  panic_if(!out, "strdup failed");

  return out;
}

static inline void *xmalloc(size_t sz)
{
  void *out = malloc(sz);

  panic_if(!out, "malloc failed");
  memset(out, 0, sz);

  return out;
}

static inline void *xrealloc(void *ptr, size_t sz)
{
  void *out = realloc(ptr, sz);

  panic_if(!out, "malloc failed");

  return out;
}

#define xsnprintf(buf, size, fmt, x...) do { \
    int r = snprintf(buf, size, fmt, x); \
    panic_if(r < 0 || r >= (int)(size), "snprintf failed for %s with %d\n", fmt, r); \
} while(0)


extern int write_file(const char *dir, const char *filename,
		const void* data, size_t len);

extern void *read_file(size_t *out_size, const char *fmt, ...);

extern const char *dir_concat(const char *dir, const char *filename);

#define xwrite_file(dir, filename, data, len) do { \
	int r = write_file(dir, filename, data, len); \
	panic_if (r != 0, "write_file failed with %d\n", r); \
} while(0)

extern int file_exists(const char *path);

extern int concat_files(const char *dst, const char *file_a, const char *file_b);

unsigned long get_aligned(unsigned long addr);

#endif /* __UTILS_H__ */
