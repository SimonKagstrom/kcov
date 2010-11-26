/*
 * Copyright (C) 2010 Simon Kagstrom, Thomas Neumann
 *
 * See COPYING for license details
 */
#ifndef __KC_ELF_H__
#define __KC_ELF_H__

struct kc_file;

struct kc_elf
{
	const char *filename;

	int n_files;
	struct kc_file *files;
};

extern struct kc_elf *kc_elf_new(const char *filename);

extern void kc_elf_add_file(struct kc_elf *elf, struct kc_file *file);

#endif
