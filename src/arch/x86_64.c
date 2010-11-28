#include <stdint.h>
#include <elf.h> /* EM_386 */

#include <kc_ptrace_arch.h>

enum
{
	x86_64_R_PC = 16,
};

static unsigned long x86_64_get_pc(struct kc *kc, void *regs_cooked)
{
	uint64_t *regs = (uint64_t *)regs_cooked;

	/* unsigned long should be the same size as uint64_t on
	 * all places where it's possible to execute x86_64 code */
	return (unsigned long)regs[x86_64_R_PC] - 1;
}

static unsigned long x86_64_setup_breakpoint(struct kc *kc,
		unsigned long addr, unsigned long old_data)
{
	unsigned long aligned_addr = get_aligned(addr);
	unsigned long offs = addr - aligned_addr;
	unsigned long shift = 8 * offs;
	unsigned long val = (old_data & ~(0xff << shift)) | (0xcc << shift);

	return val;
}

static void x86_64_adjust_pc_after_breakpoint(struct kc *kc,
		void *regs_cooked)
{
	uint64_t *regs = (uint64_t *)regs_cooked;

	regs[x86_64_R_PC] -= 1;
}

static struct kc_ptrace_arch x86_64_arch =
{
	.e_machines = (int[]){EM_X86_64, EM_NONE},
	.get_pc = x86_64_get_pc,
	.setup_breakpoint = x86_64_setup_breakpoint,
	.adjust_pc_after_breakpoint = x86_64_adjust_pc_after_breakpoint,
};
