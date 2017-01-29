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
		 * Check if an address is a valid breakpoint "point".
		 *
		 * @return true if a breakpoint can be set on this address
		 */
		virtual bool verify(const void *sectionData, size_t sectionSize,
				uint64_t offset) = 0;


		static IDisassembler &getInstance();
	};
}
