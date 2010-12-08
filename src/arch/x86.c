#include <stdint.h>
#include <elf.h> /* EM_386 */
#include <sys/user.h>

#include <kc_ptrace_arch.h>

enum
{
#if defined(__WORDSIZE) && __WORDSIZE == 64
	X86_R_PC = 16, /* i386 compatibility mode on x86_64 */
#else
	X86_R_PC = 12,
#endif
};

static unsigned long x86_get_pc(struct kc *kc, void *regs_cooked)
{
	unsigned long *regs = (unsigned long*)regs_cooked;

	return (unsigned long)regs[X86_R_PC] - 1;
}

static unsigned long x86_setup_breakpoint(struct kc *kc,
		unsigned long addr, unsigned long old_data)
{
	unsigned long aligned_addr = get_aligned(addr);
	unsigned long offs = addr - aligned_addr;
	unsigned long shift = 8 * offs;
	unsigned long val = (old_data & ~(0xffUL << shift)) |
			(0xccUL << shift);

	return val;
}

static unsigned long x86_clear_breakpoint(struct kc *kc,
		unsigned long addr, unsigned long old_data, unsigned long cur_data)
{
	unsigned long aligned_addr = get_aligned(addr);
	unsigned long offs = addr - aligned_addr;
	unsigned long shift = 8 * offs;
	unsigned long old_byte = (old_data >> shift) & 0xffUL;
	unsigned long val = (cur_data & ~(0xffUL << shift)) |
			(old_byte << shift);

	return val;
}

static void x86_adjust_pc_after_breakpoint(struct kc *kc,
		void *regs_cooked)
{
	unsigned long *regs = (unsigned long *)regs_cooked;

	regs[X86_R_PC] -= 1;
}

static struct kc_ptrace_arch x86_arch =
{
	.e_machines = (int[]){EM_386, EM_X86_64, EM_NONE},
	.get_pc = x86_get_pc,
	.setup_breakpoint = x86_setup_breakpoint,
	.adjust_pc_after_breakpoint = x86_adjust_pc_after_breakpoint,
	.clear_breakpoint = x86_clear_breakpoint,
};
