/* ARM - but no thumb support */
#include <stdint.h>
#include <elf.h>

#include <kc_ptrace_arch.h>

enum
{
	ARM_R_PC = 15, /* From asm/ptrace.h */
};

static unsigned long arm_get_pc(struct kc *kc, void *regs_cooked)
{
	uint32_t *regs = (uint32_t *)regs_cooked;

	return (unsigned long)regs[ARM_R_PC];
}

static unsigned long arm_setup_breakpoint(struct kc *kc,
		unsigned long addr, unsigned long old_data)
{
	return 0xe1200070; /* BKPT */
}

static void arm_adjust_pc_after_breakpoint(struct kc *kc,
		void *regs_cooked)
{
	uint32_t *regs = (uint32_t *)regs_cooked;

	regs[ARM_R_PC] -= 4;
}

static struct kc_ptrace_arch arm_arch =
{
	.e_machines = (int[]){EM_ARM, EM_NONE},
	.get_pc = arm_get_pc,
	.setup_breakpoint = arm_setup_breakpoint,
	.adjust_pc_after_breakpoint = arm_adjust_pc_after_breakpoint,
};
