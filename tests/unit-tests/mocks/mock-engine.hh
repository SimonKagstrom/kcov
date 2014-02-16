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
	MOCK_METHOD1(start, bool(const std::string &executable));
	MOCK_METHOD0(childrenLeft, bool());
	MOCK_METHOD0(waitEvent, const Event());
	MOCK_METHOD1(continueExecution, void(const Event));
	MOCK_METHOD1(eventToName, std::string(Event ev));
	MOCK_METHOD0(kill, void());

	unsigned int matchFile(uint8_t *data, size_t dataSize)
	{
		return match_perfect;
	}
};
