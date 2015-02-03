#pragma once

#include "../test.hh"

#include <engine.hh>

using namespace kcov;


class MockEngine : public IEngine
{
public:
	MockEngine()
	{
	}

	MAKE_MOCK1(registerBreakpoint, int(unsigned long addr));
	MAKE_MOCK2(start, bool(IEventListener &listener, const std::string &executable));
	MAKE_MOCK0(continueExecution, bool());
	MAKE_MOCK1(kill, void(int));

	unsigned int matchFile(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		return match_perfect;
	}
};
