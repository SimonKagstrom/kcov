#pragma once

#include "../test.hh"

#include <engine.hh>

using namespace kcov;

class MockEngine : public IEngine
{
public:
	MockEngine()
	{
		IEngineFactory::getInstance().registerEngine(*this);
	}

	MOCK_METHOD1(registerBreakpoint, int(unsigned long addr));
	MOCK_METHOD1(clearBreakpoint, bool(int id));
	MOCK_METHOD0(setupAllBreakpoints, void());
	MOCK_METHOD0(clearAllBreakpoints, void());
	MOCK_METHOD2(start, bool(IEventListener &listener, const std::string &executable));
	MOCK_METHOD0(continueExecution, bool());
	MOCK_METHOD1(kill, void(int));

	unsigned int matchFile(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		return match_perfect;
	}
};
