#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <libelf.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <link.h>
#include <dlfcn.h>

#include <phdr_data.h>

static struct phdr_data *phdr_data;

static int phdrCallback(struct dl_phdr_info *info, size_t size, void *data)
{
	phdr_data_add(phdr_data, info);

	return 0;
}

static int phdrSizeCallback(struct dl_phdr_info *info, size_t size, void *data)
{
	size_t *ps = (size_t *)data;

	*ps += sizeof(struct phdr_data_entry);

	return 0;
}

static void parse_solibs(void)
{
	char *kcov_solib_path;
	void *p;
	ssize_t written;
	size_t allocSize;
	size_t sz;
	int fd;

	kcov_solib_path = getenv("KCOV_SOLIB_PATH");
	if (!kcov_solib_path)
		return;

	allocSize = sizeof(struct phdr_data);
	dl_iterate_phdr(phdrSizeCallback, &allocSize);

	phdr_data = phdr_data_new(allocSize);
	if (!phdr_data) {
		fprintf(stderr, "kcov-solib: Can't allocate %zu bytes\n", allocSize);
		return;
	}


	dl_iterate_phdr(phdrCallback, NULL);

	p = phdr_data_marshal(phdr_data, &sz);

	fd = open(kcov_solib_path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "kcov-solib: Can't open %s\n", kcov_solib_path);
		return;
	}
	written = write(fd, p, sz);

	if (written != sz)
		fprintf(stderr, "kcov-solib: Can't write to solib FIFO (%zu)\n", written);

	phdr_data_free(p);

	close(fd);
}

static void *(*orig_dlopen)(const char *, int);
void *dlopen(const char *filename, int flag)
{
	void *out;

	if (!orig_dlopen)
		orig_dlopen = dlsym(RTLD_NEXT, "dlopen");

	out = orig_dlopen(filename, flag);

	parse_solibs();

	return out;
}


void  __attribute__((constructor))kcov_solib_at_startup(void)
{
	parse_solibs();
}
