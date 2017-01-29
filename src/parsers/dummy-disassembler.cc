#include <disassembler.hh>

using namespace kcov;

class DummyDisassembler : public IDisassembler
{
public:
	virtual void setup(const void *header, size_t headerSize)
	{
	}

	bool verify(const void *sectionData, size_t sectionSize, uint64_t offset)
	{
		/*
		 * ARM and PowerPC have fixed-length instructions, so this is actually
		 * a valid way of validating.
		 *
		 * Of course, it would be better if we knew if thumb mode was used.
		 */
#if defined(__arm__)
		return (offset & 1) == 0; // Thumb-conservative
#elif defined(__powerpc__)
		return (offset & 3) == 0;
#endif

		return true;
	}
};

IDisassembler &IDisassembler::getInstance()
{
	static DummyDisassembler *g_p;

	if (!g_p)
		g_p = new DummyDisassembler();

	return *g_p;
}
