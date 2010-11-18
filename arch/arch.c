#include <kc_ptrace_arch.h>
#include <elf.h> /* EM_NUM */
#include <utils.h>

/* Allow for lots of architectures! */
static struct kc_ptrace_arch *archs[EM_NUM];

struct kc_ptrace_arch *kc_ptrace_arch_get(unsigned int e_machine)
{
	if (e_machine >= EM_NUM)
		return NULL;

	return archs[e_machine];
}

void kc_ptrace_arch_register(struct kc_ptrace_arch *arch)
{
	int *machine_p;

	for (machine_p = arch->e_machines; *machine_p != EM_NONE; machine_p++) {
		int cur = *machine_p;

		panic_if(cur > EM_NUM,
				"Trying to register machine number %d when there are at most %d\n",
				cur, EM_NUM);
		archs[cur] = arch;
	}
}

void kc_ptrace_arch_setup(void)
{
}
