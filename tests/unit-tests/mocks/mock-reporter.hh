#pragma once

#include <reporter.hh>

namespace kcov
{
	class MockReporter : public IReporter
	{
	public:
		MOCK_METHOD1(fileIsIncluded,
				bool(const std::string &file));

		MOCK_METHOD2(lineIsCode,
				bool(const std::string &file, unsigned int lineNr));

		MOCK_METHOD2(getLineExecutionCount,
				LineExecutionCount(const std::string &file, unsigned int lineNr));

		MOCK_METHOD0(getExecutionSummary,
				ExecutionSummary());

		MOCK_METHOD1(registerListener, void(kcov::IReporter::IListener &listener));

		MOCK_METHOD1(marshal, void *(size_t *szOut));

		MOCK_METHOD2(unMarshal, bool(void *data, size_t sz));

		MOCK_METHOD0(stop, void());

		void *mockMarshal(size_t *outSz)
		{
			void *out = malloc(32);

			*outSz = 32;

			return out;
		}
	};
}
