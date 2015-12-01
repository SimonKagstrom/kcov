#pragma once

#include <string>
#include <vector>

#include <elfutils/libdw.h>
#include <file-parser.hh>

namespace kcov
{
	class DwarfParser
	{
	public:
		DwarfParser();

		~DwarfParser();

		bool open(const std::string &filename);

		void forEachLine(IFileParser::ILineListener &listener);

		void forAddress(IFileParser::ILineListener &listener, uint64_t address);

	private:
		std::string fullPath(const char *const *srcDirs, const std::string &filename);

		void close();

		int m_fd;
		Dwarf *m_dwarf;
	};
}
