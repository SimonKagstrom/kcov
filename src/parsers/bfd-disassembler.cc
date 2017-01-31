#include <disassembler.hh>
#include <utils.hh>

#include <unordered_map>
#include <map>

#ifndef ATTRIBUTE_FPTR_PRINTF_2
# define ATTRIBUTE_FPTR_PRINTF_2
#endif

#include <bfd.h>
#include <dis-asm.h>

#include <elf.h>

using namespace kcov;

class BfdDisassembler : public IDisassembler
{
public:
	BfdDisassembler()
	{
		init_disassemble_info(&m_info, (void *)this, BfdDisassembler::fprintFuncStatic);
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

	void addSection(const void *sectionData, size_t sectionSize, uint64_t baseAddress)
	{
		SectionCache_t::iterator it = m_cache.find(baseAddress);

		// Not visited before
		if (it == m_cache.end())
			m_cache[baseAddress] = new Section(sectionData, sectionSize, baseAddress);
	}

	bool verify(uint64_t address)
	{
		Section *p = lookupSection(address);

		if (!p)
			return true;

		if (!p->isDisassembled())
			p->disassemble(*this, m_info, m_disassembler);

		// The address is valid there is an instruction starting at it
		return getInstruction(address) != NULL;
	}

private:
	class Instruction
	{
	public:
		Instruction(bool isBranch = false, uint64_t branchTarget = 0) :
			m_isBranch(isBranch), m_branchTarget(branchTarget)
		{
		}

	private:
		bool m_isBranch;
		uint64_t m_branchTarget;
	};

	class Section
	{
	public:
		Section(const void *data, size_t size, uint64_t startAddress) :
			m_size(size),
			m_startAddress(startAddress),
			m_disassembled(false)
		{
			m_data = xmalloc(size);
			memcpy(m_data, data, size);
		}

		uint64_t getBase() const
		{
			return m_startAddress;
		}

		size_t getSize() const
		{
			return m_size;
		}

		bool isDisassembled() const
		{
			return m_disassembled;
		}

		void disassemble(BfdDisassembler &target, struct disassemble_info info, disassembler_ftype disassembler)
		{
			if (m_disassembled)
					return;

			m_disassembled = true;

			info.buffer_vma = 0;
			info.buffer_length = m_size;
			info.buffer = (bfd_byte *)m_data;
			info.stream = (void *)this;

			uint64_t pc = 0;
			int count;
			do
			{
				count = disassembler(pc, &info);

				if (count < 0)
					break;

				target.m_instructions[pc + m_startAddress] = Instruction();

				pc += count;
			} while (count > 0 && pc < m_size);
		}

	private:
		void *m_data;
		const size_t m_size;
		const uint64_t m_startAddress;

		bool m_disassembled; // Lazy disassembly once it's used
	};

	Section *lookupSection(uint64_t address)
	{
		SectionCache_t::iterator it = m_cache.lower_bound(address);

		if (it == m_cache.end())
			return NULL;
		--it;

		// Above the last symbol
		if (it == m_cache.end())
			return NULL;

		Section *cur = it->second;
		uint64_t end = cur->getBase() + cur->getSize() - 1;

		// Below nearest (possible?)
		if (address < cur->getBase())
			return NULL;

		// Above section
		if (address > end)
			return NULL;

		// Just right!
		return cur;
	}

	Instruction *getInstruction(uint64_t address)
	{
		if (m_instructions.find(address) == m_instructions.end())
			return NULL;

		return &m_instructions[address];
	}

	static int fprintFuncStatic(void *info, const char *fmt, ...)
	{
		// Do nothing - we're not interested in the actual encoding
		return 0;
	}
	typedef std::map<uint64_t, Section *> SectionCache_t;
	typedef std::unordered_map<uint64_t, Instruction> InstructionAddressMap_t;

	struct disassemble_info m_info;
	disassembler_ftype m_disassembler;

	SectionCache_t m_cache;
	InstructionAddressMap_t m_instructions;
};

IDisassembler &IDisassembler::getInstance()
{
	static BfdDisassembler *g_p;

	if (!g_p) {
		bfd_init();
		g_p = new BfdDisassembler();
	}

	return *g_p;
}
