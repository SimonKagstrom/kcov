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

	private:
		void close();

		int m_fd;
		Dwarf *m_dwarf;
	};
}
