#pragma once

#include <sys/types.h>
#include <stdint.h>

namespace kcov
{
	class IElf
	{
	public:
		class IListener
		{
		public:
			virtual void onLine(const char *file, unsigned int lineNr,
					unsigned long addr) = 0;
		};

		static IElf *open(const char *filename);

		static IElf &getInstance();


		virtual bool addFile(const char *filename) = 0;

		virtual const char *getFilename() = 0;

		virtual void registerListener(IListener &listener) = 0;

		virtual bool parse() = 0;

		virtual uint64_t getChecksum() = 0;
	};
}
