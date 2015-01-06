#pragma once

#include <reporter.hh>

namespace kcov
{
	class MockReporter : public IReporter
	{
	public:
		MAKE_MOCK1(fileIsIncluded,
				bool(const std::string &file));

		MAKE_MOCK2(lineIsCode,
				bool(const std::string &file, unsigned int lineNr));

		MAKE_MOCK2(getLineExecutionCount,
				LineExecutionCount(const std::string &file, unsigned int lineNr));

		MAKE_MOCK0(getExecutionSummary,
				ExecutionSummary());

		MAKE_MOCK1(registerListener, void(kcov::IReporter::IListener &listener));

		MAKE_MOCK1(marshal, void *(size_t *szOut));

		MAKE_MOCK2(unMarshal, bool(void *data, size_t sz));

		MAKE_MOCK0(stop, void());

		void *mockMarshal(size_t *outSz)
		{
			void *out = malloc(32);

			*outSz = 32;

			return out;
		}
	};
}
