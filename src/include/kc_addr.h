/*
 * Copyright (C) 2010 Simon Kagstrom, Thomas Neumann
 *
 * See COPYING for license details
 */
#ifndef __KC_ADDR_H__
#define __KC_ADDR_H__

struct kc_line;

struct kc_addr
{
	unsigned long addr;
	unsigned long hits;
	unsigned long saved_code; /* Not used by kprobe-coverage */
};

extern struct kc_addr *kc_addr_new(unsigned long addr);

extern void kc_addr_register_hit(struct kc_addr *addr);

extern void kc_addr_marshall(struct kc_addr *addr);

extern void kc_addr_unmarshall(struct kc_addr *addr);

#endif
