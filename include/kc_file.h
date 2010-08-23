#ifndef __KC_FILE_H__
#define __KC_FILE_H__

#include <glib.h>

struct kc_line;

struct kc_file
{
	const char *filename;

	GHashTable *lines;
};

extern struct kc_file *kc_file_new(const char *filename);

extern struct kc_line *kc_file_lookup_line(struct kc_file *file, int lineno);

extern void kc_file_add_line(struct kc_file *file, struct kc_line *line);

#endif
