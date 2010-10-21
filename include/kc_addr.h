#ifndef __KC_ADDR_H__
#define __KC_ADDR_H__

struct kc_line;

struct kc_addr
{
	unsigned long addr;
	unsigned int hits;
	unsigned long saved_code; /* Not used by kprobe-coverage */
};

extern struct kc_addr *kc_addr_new(unsigned long addr);

extern void kc_addr_register_hit(struct kc_addr *addr);

#endif
