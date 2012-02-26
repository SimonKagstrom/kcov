#pragma once

#include "../test.hh"

#include <engine.hh>

using namespace kcov;

class MockEngine : public IEngine {
public:
	MockEngine()
	{
	}

	MOCK_METHOD1(setBreakpoint, int(unsigned long addr));
	MOCK_METHOD1(clearBreakpoint, bool(int id));
	MOCK_METHOD0(clearAllBreakpoints, void());
	MOCK_METHOD1(start, int(const char *executable));
	MOCK_METHOD0(continueExecution, const Event());
	MOCK_METHOD0(kill, void());
};
