#pragma once

#include <stdint.h>
#include <stddef.h>

namespace kcov
{
	class IDisassembler
	{
	public:
		virtual ~IDisassembler()
		{
		}

		/**
		 * Setup the verifier with an ELF file header
		 */
		virtual void setup(const void *header, size_t headerSize) = 0;

		/**
		 * Add an executable section
		 *
		 * @param sectionData the data of the section
		 * @param sectionSize the size of the section
		 * @param base address the virtual start address
		 */
		virtual void addSection(const void *sectionData, size_t sectionSize, uint64_t baseAddress) = 0;

		/**
		 * Check if an address is a valid breakpoint "point".
		 *
		 * @return true if a breakpoint can be set on this address
		 */
		virtual bool verify(uint64_t address) = 0;


		static IDisassembler &getInstance();
	};
}
