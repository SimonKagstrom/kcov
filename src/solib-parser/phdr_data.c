#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include "phdr_data.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <link.h>

#define KCOV_MAGIC         0x6b636f76 /* "kcov" */
#define KCOV_SOLIB_VERSION 2

struct phdr_data *phdr_data_new(void)
{
	struct phdr_data *p = malloc(sizeof(struct phdr_data));

	p->magic = KCOV_MAGIC;
	p->version = KCOV_SOLIB_VERSION;
	p->n_entries = 0;

	return p;
}

void phdr_data_add(struct phdr_data **p_in, struct dl_phdr_info *info)
{
	struct phdr_data *p = *p_in;
	uint32_t n = p->n_entries + 1;
	uint32_t curEntry = p->n_entries;
	int phdr;

	p = realloc(p, sizeof(struct phdr_data) + n * sizeof(struct phdr_data_entry));
	if (!p) {
		fprintf(stderr, "Can't allocate phdr data");

		return;
	}

	struct phdr_data_entry *cur = &p->entries[curEntry];

	memset(cur, 0, sizeof(*cur));
	strncpy(cur->name, info->dlpi_name, sizeof(cur->name) - 1);

	cur->n_segments = 0;
	for (phdr = 0; phdr < info->dlpi_phnum; phdr++) {
		const ElfW(Phdr) *curHdr = &info->dlpi_phdr[phdr];
		struct phdr_data_segment *seg = &cur->segments[cur->n_segments];

		if (curHdr->p_type != PT_LOAD)
			continue;

		if (cur->n_segments >= sizeof(cur->segments) / sizeof(cur->segments[0]))
		{
			fprintf(stderr, "Too many segments\n");
			return;
		}

		seg->paddr = curHdr->p_paddr;
		seg->vaddr = info->dlpi_addr + curHdr->p_vaddr;
		seg->size = curHdr->p_memsz;

		cur->n_segments++;
	}

	p->n_entries = n;

	*p_in = p;
}

void *phdr_data_marshal(struct phdr_data *p, size_t *out_sz)
{
	*out_sz = sizeof(struct phdr_data) + sizeof(struct phdr_data_entry) * p->n_entries;

	return (void *)p;
}

struct phdr_data *phdr_data_unmarshal(void *p)
{
	struct phdr_data *out = (struct phdr_data *)p;

	if (out->magic != KCOV_MAGIC)
		return NULL;
	if (out->version != KCOV_SOLIB_VERSION)
		return NULL;

	return out;
}
