#pragma once

#include <stdint.h>

#include <functional>
#include <string>

namespace kcov
{
	class LineId
	{
	public:
		LineId(const std::string &fileName, unsigned int nr) :
			m_hash(std::hash<std::string>()(fileName) ^ std::hash<unsigned int>()(nr))
		{
		}

		bool operator==(const LineId &other) const
		{
			return m_hash == other.m_hash;
		}

		const size_t m_hash;
	};

	class LineIdHash
	{
	public:
		size_t operator()(const LineId &obj) const
		{
			return obj.m_hash;
		}
	};

}
