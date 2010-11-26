/*
 * Copyright (C) 2010 Simon Kagstrom, Thomas Neumann
 *
 * See COPYING for license details
 */
#include <kc_file.h>
#include <kc_line.h>
#include <utils.h>


struct kc_file *kc_file_new(const char *filename)
{
	struct kc_file *out = xmalloc(sizeof(struct kc_file));

	out->filename = xstrdup(filename);
	out->lines = g_hash_table_new(g_int_hash, g_int_equal);
	out->percentage = 0;

	return out;
}

void kc_file_add_line(struct kc_file *file, struct kc_line *line)
{
	g_hash_table_insert(file->lines, &line->lineno, line);
}

struct kc_line *kc_file_lookup_line(struct kc_file *file, int lineno)
{
	return g_hash_table_lookup(file->lines, &lineno);
}
