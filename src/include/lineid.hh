#pragma once

#include <stdint.h>

#include <string>

namespace kcov
{
	class LineId
	{
	public:
		LineId(const std::string &fileName, int nr) :
			m_file(fileName), m_lineNr(nr)
		{
		}

		bool operator==(const LineId &other) const
		{
			return (m_file == other.m_file) && (m_lineNr == other.m_lineNr);
		}

		const std::string m_file;
		unsigned int m_lineNr;
	};

	class LineIdHash
	{
	public:
		size_t operator()(const LineId &obj) const
		{
			return std::hash<std::string>()(obj.m_file) ^ std::hash<int>()(obj.m_lineNr);
		}
	};

}
