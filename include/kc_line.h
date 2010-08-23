#ifndef __KC_LINE_H__
#define __KC_LINE_H__

struct kc_addr;
struct kc_file;

typedef struct kc_line
{
	unsigned int possible_hits;
	int lineno;
	struct kc_file *file;

	int n_addrs;
	struct kc_addr **addrs;
} kc_line_t;

extern struct kc_line *kc_line_new(struct kc_file *file, int lineno);

extern void kc_line_free(struct kc_line *);

extern void kc_line_add_addr(struct kc_line *line, struct kc_addr *addr);

#endif
