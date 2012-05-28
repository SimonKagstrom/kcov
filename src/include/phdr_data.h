#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct phdr_data_entry
{
	char name[1024];

	unsigned long paddr;
	unsigned long vaddr;
	unsigned long size;
};

struct phdr_data
{
	uint32_t magic;
	uint32_t version;
	uint32_t n_entries;

	struct phdr_data_entry entries[];
};

struct phdr_data *phdr_data_new(void);

void phdr_data_add(struct phdr_data **p,
		unsigned long paddr, unsigned long vaddr, unsigned long size,
		const char *name);

void *phdr_data_marshal(struct phdr_data *p, size_t *out_sz);

struct phdr_data *phdr_data_unmarshal(void *p);

#ifdef __cplusplus
}
#endif
