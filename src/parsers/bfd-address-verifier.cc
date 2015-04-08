#include "address-verifier.hh"
#include <utils.hh>

#include <unordered_map>

#ifndef ATTRIBUTE_FPTR_PRINTF_2
# define ATTRIBUTE_FPTR_PRINTF_2
#endif

#include <bfd.h>
#include <dis-asm.h>

#include <elf.h>

using namespace kcov;

class BfdAddressVerifier : public IAddressVerifier
{
	typedef std::unordered_map<uint64_t, bool> InstructionAddressMap_t;
	typedef std::unordered_map<const void *, InstructionAddressMap_t> SectionCache_t;

public:
	BfdAddressVerifier()
	{
		init_disassemble_info(&m_info, (void *)this, BfdAddressVerifier::fprintFuncStatic);
		m_disassembler = print_insn_i386;

		m_info.arch = bfd_arch_i386;

		disassemble_init_for_target(&m_info);
		m_info.application_data = (void *)this;
	}

	virtual void setup(const void *header, size_t headerSize)
	{
		const uint8_t *data = (const uint8_t *)header;

		panic_if(headerSize <= EI_CLASS,
				"Header size must be at least %u", EI_CLASS);

		if (data[EI_CLASS] == ELFCLASS64)
			m_info.mach = bfd_mach_x86_64;
		else
			m_info.mach = bfd_mach_i386_i386;
	}

	bool verify(const void *sectionData, size_t sectionSize, uint64_t offset)
	{
		SectionCache_t::iterator it = m_cache.find(sectionData);

		// Not visited before, disassemble
		if (it == m_cache.end()) {
			// Insert and reference it
			InstructionAddressMap_t &cur = m_cache[sectionData];

			doDisassemble(cur, sectionData, sectionSize);

			it = m_cache.find(sectionData);
		}

		return it->second.find(offset) != it->second.end();
	}
private:
	void doDisassemble(InstructionAddressMap_t &insnMap, const void *p, size_t size)
	{
		uint8_t *data = (uint8_t *)p;

		if (!data || size == 0)
			return;

		m_info.buffer_vma = 0;
		m_info.buffer_length = size;
		m_info.buffer = (bfd_byte *)p;
		m_info.stream = (void *)this;

		uint64_t pc = 0;
		int count;
		do
		{
			count = m_disassembler(pc, &m_info);

			if (count < 0)
				break;

			insnMap[pc] = true;

			pc += count;
		} while (count > 0 && pc < size);
	}

	static int fprintFuncStatic(void *info, const char *fmt, ...)
	{
		// Do nothing - we're not interested in the actual encoding
		return 0;
	}

	struct disassemble_info m_info;
	disassembler_ftype m_disassembler;

	SectionCache_t m_cache;
};

IAddressVerifier *IAddressVerifier::create()
{
	static bool g_bfdInited;

	if (!g_bfdInited) {
		bfd_init();
		g_bfdInited = true;
	}

	return new BfdAddressVerifier();
}
