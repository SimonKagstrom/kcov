#include <kc_addr.h>
#include <utils.h>

struct kc_addr *kc_addr_new(unsigned long addr)
{
	struct kc_addr *out = xmalloc(sizeof(struct kc_addr));

	out->hits = 0;
	out->addr = addr;
	out->saved_code = 0;

	return out;
}

void kc_addr_register_hit(struct kc_addr *addr)
{
	addr->hits++;
}
