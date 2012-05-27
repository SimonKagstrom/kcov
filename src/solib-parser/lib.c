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

#include <phdr_data.h>

static struct phdr_data *phdr_data;


static int phdrCallback(struct dl_phdr_info *info, size_t size, void *data)
{
	int phdr;

	for (phdr = 0; phdr < info->dlpi_phnum; phdr++) {
		const ElfW(Phdr) *cur = &info->dlpi_phdr[phdr];

		if (cur->p_type != PT_LOAD)
			continue;

		phdr_data_add(phdr_data, cur->p_paddr, info->dlpi_addr + cur->p_vaddr,
				cur->p_memsz, info->dlpi_name);
		return 0;
	}

	return 0;
}

void  __attribute__((constructor))at_startup(void)
{
	char *kcov_solib_path;
	void *p;
	size_t sz;
	int fd;

	kcov_solib_path = getenv("KCOV_SOLIB_PATH");
	if (!kcov_solib_path)
	{
		printf("not set\n");
		return;
	}

	fd = open(kcov_solib_path, O_WRONLY);
	if (fd < 0)
	{
		printf("no open %s\n", kcov_solib_path);
		return;
	}

	phdr_data = phdr_data_new();

	dl_iterate_phdr(phdrCallback, NULL);

	p = phdr_data_marshal(phdr_data, &sz);
	write(fd, p, sz);

	free(p);

	close(fd);
}
