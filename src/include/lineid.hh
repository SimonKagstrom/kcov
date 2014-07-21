#pragma once

#include <stdint.h>

#include <functional>
#include <string>

namespace kcov
{
	static size_t getLineId(const std::string &fileName, unsigned int nr)
	{
		return std::hash<std::string>()(fileName) ^ std::hash<unsigned int>()(nr);
	}
}
