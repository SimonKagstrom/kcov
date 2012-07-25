#pragma once

#include <sys/types.h>
#include <stdint.h>

struct phdr_data_entry;

namespace kcov
{
	class IElf
	{
	public:
		class ILineListener
		{
		public:
			virtual void onLine(const char *file, unsigned int lineNr,
					unsigned long addr) = 0;
		};

		static IElf *open(const char *filename);

		static IElf &getInstance();


		virtual ~IElf() {}

		virtual bool addFile(const char *filename, struct phdr_data_entry *phdr_data = 0) = 0;

		virtual const char *getFilename() = 0;

		virtual void registerLineListener(ILineListener &listener) = 0;

		virtual bool parse() = 0;

		virtual uint64_t getChecksum() = 0;
	};
}
