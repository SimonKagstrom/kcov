#include <stdint.h>
#include <elf.h> /* EM_386 */
#include <sys/user.h>

#include <kc_ptrace_arch.h>

enum
{
#if defined(__WORDSIZE) && __WORDSIZE == 64
	I386_R_PC = 32, /* i386 compatibility mode on x86_64 */
#else
	I386_R_PC = 12,
#endif
};

static unsigned long i386_get_pc(struct kc *kc, void *regs_cooked)
{
	uint32_t *regs = (uint32_t *)regs_cooked;

	return (unsigned long)regs[I386_R_PC] - 1;
}

static unsigned long i386_setup_breakpoint(struct kc *kc,
		unsigned long addr, unsigned long old_data)
{
	unsigned long aligned_addr = get_aligned(addr);
	unsigned long offs = addr - aligned_addr;
	unsigned long shift = 8 * offs;
	unsigned long val = (old_data & ~((unsigned long)0xff << shift)) |
			((unsigned long)0xcc << shift);

	return val;
}

static unsigned long i386_clear_breakpoint(struct kc *kc,
		unsigned long addr, unsigned long old_data, unsigned long cur_data)
{
	unsigned long aligned_addr = get_aligned(addr);
	unsigned long offs = addr - aligned_addr;
	unsigned long shift = 8 * offs;
	unsigned long old_byte = (old_data >> shift) & (unsigned long)0xff;
	unsigned long val = (cur_data & ~((unsigned long)0xff << shift)) |
			(old_byte << shift);

	return val;
}

static void i386_adjust_pc_after_breakpoint(struct kc *kc,
		void *regs_cooked)
{
	uint32_t *regs = (uint32_t *)regs_cooked;

	regs[I386_R_PC] -= 1;
}

static struct kc_ptrace_arch i386_arch =
{
	.e_machines = (int[]){EM_386, EM_NONE},
	.get_pc = i386_get_pc,
	.setup_breakpoint = i386_setup_breakpoint,
	.adjust_pc_after_breakpoint = i386_adjust_pc_after_breakpoint,
	.clear_breakpoint = i386_clear_breakpoint,
};
