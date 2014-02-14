#pragma once

#include <sys/types.h>
#include <stdint.h>

struct phdr_data_entry;

namespace kcov
{
	class IFileParser
	{
	public:
		class ILineListener
		{
		public:
			virtual void onLine(const char *file, unsigned int lineNr,
					unsigned long addr) = 0;
		};

		class IFileListener
		{
		public:
			virtual void onFile(const char *file, bool isSolib) = 0;
		};

		static IFileParser *open(const char *filename);

		static IFileParser &getInstance();


		virtual ~IFileParser() {}

		virtual void registerLineListener(ILineListener &listener) = 0;

		virtual void registerFileListener(IFileListener &listener) = 0;

		virtual bool parse() = 0;

		virtual uint64_t getChecksum() = 0;
	};

	/**
	 * Parser for compiled files (ELF)
	 */
	class ICompiledFileParser : public IFileParser
	{
	public:
		virtual bool addFile(const char *filename, struct phdr_data_entry *phdr_data = 0) = 0;

		virtual bool elfIs64Bit() = 0;
	};
}
