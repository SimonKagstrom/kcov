#pragma once

#include <stdint.h>

#include <functional>
#include <string>

namespace kcov
{
	static uint64_t getFileHash(const std::string &fileName)
	{
		return std::hash<std::string>()(fileName);
	}

	static uint64_t getLineId(const std::string &fileName, unsigned int nr)
	{
		return (getFileHash(fileName) << 32) | ((uint64_t)nr);
	}
}
