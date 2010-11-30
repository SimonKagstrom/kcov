/*
 * Copyright (C) 2010 Simon Kagstrom
 *
 * See COPYING for license details
 */
#include <stdio.h>
#include <utils.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

void *read_file(size_t *out_size, const char *fmt, ...)
{
	struct stat buf;
	char path[2048];
	va_list ap;
	void *data;
	size_t size;
	FILE *f;
	int r;

	/* Create the filename */
	assert ( fmt != NULL );
	va_start(ap, fmt);
	r = vsnprintf(path, 2048, fmt, ap);
	va_end(ap);

	if (lstat(path, &buf) < 0)
		return NULL;

	size = buf.st_size;
	data = xmalloc(size + 2); /* NULL-terminate, if used as string */
	f = fopen(path, "r");
	if (!f)
	{
		free(data);
		return NULL;
	}
	if (fread(data, 1, size, f) != size)
	{
		free(data);
		data = NULL;
	}
	fclose(f);

	*out_size = size;

	return data;
}

int write_file(const void *data, size_t len, const char *fmt, ...)
{
	char path[2048];
	va_list ap;
	FILE *fp;
	int ret = 0;

	/* Create the filename */
	assert ( fmt != NULL );
	va_start(ap, fmt);
	vsnprintf(path, 2048, fmt, ap);
	va_end(ap);

	fp = fopen(path, "w");
	if (!fp)
		return -1;

	if (fwrite(data, sizeof(uint8_t), len, fp) != len)
		ret = -1;
	fclose(fp);

	return ret;
}

const char *dir_concat(const char *dir, const char *filename)
{
	size_t len = strlen(dir) + strlen(filename) + 4;
	char *out = xmalloc(len);

	xsnprintf(out, len, "%s/%s", dir, filename);

	return out;
}

int file_exists(const char *path)
{
	struct stat st;

	return lstat(path, &st) == 0;
}

static void read_write(FILE *dst, FILE *src)
{
	char buf[1024];

	while (!feof(src)) {
		int n = fread(buf, sizeof(char), sizeof(buf), src);

		fwrite(buf, sizeof(char), n, dst);
	}
}

int concat_files(const char *dst_name, const char *file_a, const char *file_b)
{
	FILE *dst;
	FILE *s1, *s2;
	int ret = -1;

	dst = fopen(dst_name, "w");
	if (!dst)
		return -1;
	s1 = fopen(file_a, "r");
	if (!s1)
		goto out_dst;
	s2 = fopen(file_b, "r");
	if (!s2)
		goto out_s1;

	read_write(dst, s1);
	read_write(dst, s2);

	fclose(s2);
out_s1:
	fclose(s1);
out_dst:
	fclose(dst);

	return ret;
}

unsigned long get_aligned(unsigned long addr)
{
	return (addr / sizeof(long)) * sizeof(long);
}

unsigned long get_aligned_4b(unsigned long addr)
{
	return addr & ~3;
}
