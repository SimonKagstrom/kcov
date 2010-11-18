#ifndef KC_PTRACE_ARCH_H
#define KC_PTRACE_ARCH_H

#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <sys/types.h>
#include <unistd.h>

struct kc;

struct kc_ptrace_arch
{
	/**
	 * NULL-terminated list of compatible architectures
	 * (from elf.h)
	 */
	int *e_machines;

	/**
	 * Return the current program counter
	 *
	 * @param kc the KCOV context
	 * @return the current program counter
	 */
	unsigned long (*get_pc)(struct kc *kc, void *regs_cooked);

	/**
	 * Setup a breakpoint
	 *
	 * @param kc the KCOV context
	 * @param addr the address to setup the breakpoint at
	 */
	unsigned long (*setup_breakpoint)(struct kc *kc, unsigned long addr, unsigned long old_data);

	void (*adjust_pc_after_breakpoint)(struct kc *kc, void *regs_cooked);
};

extern void kc_ptrace_arch_register(struct kc_ptrace_arch *arch);

extern struct kc_ptrace_arch *kc_ptrace_arch_get(unsigned int e_machine);

extern void kc_ptrace_arch_setup(void);

#endif
