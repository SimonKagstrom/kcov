#pragma once

#include <string>
#include <vector>

namespace kcov
{
	class IFileParser;

	class IDatabaseCreator
	{
	public:
		virtual ~IDatabaseCreator()
		{
		}

		virtual void write() = 0;

		static IDatabaseCreator &create(IFileParser &parser);
	};

	class IDatabaseReader
	{
	public:
		virtual ~IDatabaseReader()
		{
		}

		/**
		 * Lookup addresses for a particular file (checksum)
		 *
		 * Borrowed from the implementation. Valid until the next get() or destruction.
		 */
		virtual const std::vector<uint64_t> &get(uint64_t checksum) = 0;

		virtual bool exists(uint64_t checksum) = 0;

		static IDatabaseReader &getInstance();
	};
}
