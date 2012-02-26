#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define error(x...) do \
{ \
	fprintf(stderr, "Error: "); \
	fprintf(stderr, x); \
	fprintf(stderr, "\n"); \
} while(0)

#define warning(x...) do \
{ \
	fprintf(stderr, "Warning: "); \
	fprintf(stderr, x); \
	fprintf(stderr, "\n"); \
} while(0)

#define panic(x...) do \
{ \
	error(x); \
	exit(1); \
} while(0)

enum debug_mask
{
	INFO_MSG   = 1,
	PTRACE_MSG = 2,
	ELF_MSG    = 4,
	BP_MSG     = 8,
};
extern int g_kcov_debug_mask;

static inline void kcov_debug(enum debug_mask dbg, const char *fmt, ...) __attribute__((format(printf,2,3)));

static inline void kcov_debug(enum debug_mask dbg, const char *fmt, ...)
{
	va_list ap;

	if ((g_kcov_debug_mask & dbg) == 0)
		return;

	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

#define panic_if(cond, x...) \
		do { if ((cond)) panic(x); } while(0)

static inline char *xstrdup(const char *s)
{
	char *out = strdup(s);

	panic_if(!out, "strdup failed");

	return out;
}

extern int file_is_elf(const char *filename);

static inline void *xmalloc(size_t sz)
{
  void *out = malloc(sz);

  panic_if(!out, "malloc failed");
  memset(out, 0, sz);

  return out;
}

extern int write_file(const void *data, size_t len, const char *fmt, ...);

extern void *read_file(size_t *out_size, const char *fmt, ...);

extern const char *dir_concat(const char *dir, const char *filename);

#define xwrite_file(data, len, dir...) do { \
	int r = write_file(data, len, dir); \
	panic_if (r != 0, "write_file failed with %d\n", r); \
} while(0)

#define xsnprintf(buf, size, fmt, x...) do { \
    int r = snprintf(buf, size, fmt, x); \
    panic_if(r < 0 || r >= (int)(size), "snprintf failed for %s with %d\n", fmt, r); \
} while(0)


extern int file_exists(const char *path);

extern int concat_files(const char *dst, const char *file_a, const char *file_b);

/* Expand ~ etc in a path */
extern const char *expand_path(const char *path);

unsigned long get_aligned(unsigned long addr);

unsigned long get_aligned_4b(unsigned long addr);
