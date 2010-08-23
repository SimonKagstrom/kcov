#include <kc_line.h>
#include <kc_addr.h>
#include <utils.h>

struct kc_line *kc_line_new(struct kc_file *file, int lineno)
{
	struct kc_line *out = xmalloc(sizeof(struct kc_line));

	out->file = file;
	out->possible_hits = 0;
	out->lineno = lineno;
	out->n_addrs = 0;
	out->addrs = NULL;

	return out;
}

void kc_line_free(struct kc_line *line)
{
	free(line);
}


void kc_line_add_addr(struct kc_line *line, struct kc_addr *addr)
{
	int cur = line->n_addrs;

	line->possible_hits++;
	line->n_addrs++;
	line->addrs = xrealloc(line->addrs, line->n_addrs * sizeof(void *));

	line->addrs[cur] = addr;
}
