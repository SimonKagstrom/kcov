#include <stdint.h>
#include <elf.h> /* EM_386 */

#include <kc_ptrace_arch.h>

enum
{
	MIPS32_R_PC = 34,
};

static unsigned long mips32_get_pc(struct kc *kc, void *regs_cooked)
{
	uint32_t *regs = (uint32_t *)regs_cooked;

	return (unsigned long)regs[MIPS32_R_PC];
}

static unsigned long mips32_setup_breakpoint(struct kc *kc,
		unsigned long addr, unsigned long old_data)
{
	return 0xd;
}

static void mips32_adjust_pc_after_breakpoint(struct kc *kc,
		void *regs_cooked)
{
	uint32_t *regs = (uint32_t *)regs_cooked;

	regs[MIPS32_R_PC] -= 4;
}

static struct kc_ptrace_arch mips32_arch =
{
	.e_machines = (int[]){EM_MIPS, EM_MIPS_RS3_LE, EM_NONE},
	.get_pc = mips32_get_pc,
	.setup_breakpoint = mips32_setup_breakpoint,
	.adjust_pc_after_breakpoint = mips32_adjust_pc_after_breakpoint,
};
