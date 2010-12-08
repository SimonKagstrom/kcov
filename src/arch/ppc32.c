#include <stdint.h>
#include <elf.h>

#include <kc_ptrace_arch.h>

enum
{
	PPC32_R_PC = 33, /* From asm/ptrace.h */
};

static unsigned long ppc32_get_pc(struct kc *kc, void *regs_cooked)
{
	uint32_t *regs = (uint32_t *)regs_cooked;

	return (unsigned long)regs[PPC32_R_PC];
}

static unsigned long ppc32_setup_breakpoint(struct kc *kc,
		unsigned long addr, unsigned long old_data)
{
	return 0x7fe00008; /* trap */
}

static void ppc32_adjust_pc_after_breakpoint(struct kc *kc,
		void *regs_cooked)
{
	uint32_t *regs = (uint32_t *)regs_cooked;

	regs[PPC32_R_PC] -= 4;
}

static struct kc_ptrace_arch ppc32_arch =
{
	.e_machines = (int[]){EM_PPC, EM_NONE},
	.get_pc = ppc32_get_pc,
	.setup_breakpoint = ppc32_setup_breakpoint,
	.adjust_pc_after_breakpoint = ppc32_adjust_pc_after_breakpoint,
};
