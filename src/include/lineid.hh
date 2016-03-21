#pragma once

#include <stdint.h>

#include <functional>
#include <string>

namespace kcov
{
	static inline uint64_t getFileHash(const std::string &fileName)
	{
		return std::hash<std::string>()(fileName);
	}

	static inline uint64_t getLineId(uint64_t fileHash, unsigned int nr)
	{
		return (fileHash << 32) | ((uint64_t)nr);
	}

	static inline uint64_t getLineId(const std::string &fileName, unsigned int nr)
	{
		return getLineId(getFileHash(fileName), nr);
	}
}
