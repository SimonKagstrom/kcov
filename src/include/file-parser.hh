#pragma once

#include <manager.hh>

#include <sys/types.h>
#include <stdint.h>

#include <string>

struct phdr_data_entry;

namespace kcov
{
	class IFilter;

	class IFileParser
	{
	public:
		enum FileFlags
		{
			FLG_NONE = 0,
			FLG_TYPE_SOLIB = 1,
		};

		class ILineListener
		{
		public:
			virtual void onLine(const std::string &file, unsigned int lineNr,
					unsigned long addr) = 0;
		};

		class IFileListener
		{
		public:
			virtual void onFile(const std::string &file, enum FileFlags flags) = 0;
		};

		virtual bool addFile(const std::string &filename, struct phdr_data_entry *phdr_data = 0) = 0;

		virtual ~IFileParser() {}

		virtual void registerLineListener(ILineListener &listener) = 0;

		virtual void registerFileListener(IFileListener &listener) = 0;

		virtual bool parse() = 0;

		virtual uint64_t getChecksum() = 0;

		virtual std::string getParserType() = 0;


		virtual unsigned int matchParser(const std::string &filename, uint8_t *data, size_t dataSize) = 0;

		// Setup once the parser has been chosen
		virtual void setupParser(IFilter *filter) = 0;
	};


	/**
	 * Manager class for getting one of multiple parsers, which can
	 * match different file types.
	 */
	class IParserManager
	{
	public:
		virtual ~IParserManager()
		{
		}

		virtual void registerParser(IFileParser &parser) = 0;

		virtual IFileParser *matchParser(const std::string &file) = 0;

		static IParserManager &getInstance();
	};
}
