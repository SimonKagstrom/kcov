#pragma once

#include <stddef.h>

namespace kcov
{
	class IFileParser;
	class ICollector;

	class IReporter
	{
	public:
		class LineExecutionCount
		{
		public:
			LineExecutionCount(unsigned int hits, unsigned int possibleHits) :
				m_hits(hits), m_possibleHits(possibleHits)
			{
			}

			unsigned int m_hits;
			unsigned int m_possibleHits;
		};

		class ExecutionSummary
		{
		public:
			ExecutionSummary() : m_lines(0), m_executedLines(0)
			{
			}

			ExecutionSummary(unsigned int lines, unsigned int executedLines) :
				m_lines(lines), m_executedLines(executedLines)
			{
			}

			unsigned int m_lines;
			unsigned int m_executedLines;
		};

		virtual ~IReporter() {}

		virtual bool lineIsCode(const char *file, unsigned int lineNr) = 0;

		virtual LineExecutionCount getLineExecutionCount(const char *file, unsigned int lineNr) = 0;

		virtual ExecutionSummary getExecutionSummary() = 0;

		virtual void *marshal(size_t *szOut) = 0;

		virtual bool unMarshal(void *data, size_t sz) = 0;

		virtual void stop() = 0;

		static IReporter &create(IFileParser &elf, ICollector &collector);
	};
}
