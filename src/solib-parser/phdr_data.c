#include "phdr_data.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define KCOV_MAGIC         0x6b636f76 /* "kcov" */
#define KCOV_SOLIB_VERSION 1

struct phdr_data *phdr_data_new(void)
{
	struct phdr_data *p = malloc(sizeof(struct phdr_data));

	p->magic = KCOV_MAGIC;
	p->version = KCOV_SOLIB_VERSION;
	p->n_entries = 0;

	return p;
}

void phdr_data_add(struct phdr_data *p,
		unsigned long paddr, unsigned long vaddr, unsigned long size,
		const char *name)
{
	uint32_t n = p->n_entries + 1;
	struct phdr_data_entry *cur = &p->entries[p->n_entries];

	p = realloc(p, sizeof(struct phdr_data) + n * sizeof(struct phdr_data_entry));
	if (!p) {
		fprintf(stderr, "Can't allocate phdr data");

		return;
	}

	memset(cur, 0, sizeof(*cur));
	strncpy(cur->name, name, sizeof(cur->name) - 1);
	cur->paddr = paddr;
	cur->vaddr = vaddr;
	cur->size = size;

	p->n_entries = n;
}

void *phdr_data_marshal(struct phdr_data *p, size_t *out_sz)
{
	*out_sz = sizeof(struct phdr_data) + sizeof(struct phdr_data_entry) * p->n_entries;

	return (void *)p;
}

struct phdr_data *phdr_data_unmarshal(void *p)
{
	return (struct phdr_data *)p;
}
