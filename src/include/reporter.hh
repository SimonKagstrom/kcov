#pragma once

#include <string>

#include <stddef.h>

namespace kcov
{
	class IFileParser;
	class ICollector;
	class IFilter;

	/**
	 * Interface class that reports addresses pairs being executed.
	 *
	 * Can also be queried for code lines etc.
	 */
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
			ExecutionSummary() : m_lines(0), m_executedLines(0), m_includeInTotals(true)
			{
			}

			ExecutionSummary(unsigned int lines, unsigned int executedLines) :
				m_lines(lines), m_executedLines(executedLines), m_includeInTotals(true)
			{
			}

			unsigned int m_lines;
			unsigned int m_executedLines;
			unsigned int m_includeInTotals;
		};

		/**
		 * Listener to executed addresses.
		 */
		class IListener
		{
		public:
			/**
			 * An address has been executed by the covered program. Can also
			 * report data from previous runs.
			 *
			 * @param addr the executed address
			 * @param hits the number of hits of the address (typically 1)
			 */
			virtual void onAddress(unsigned long addr, unsigned long hits) = 0;
		};


		virtual ~IReporter() {}


		/**
		 * Register a listener for reported addresses.
		 *
		 * @param listener the listener
		 */
		virtual void registerListener(IListener &listener) = 0;

		/**
		 * Return if a file path should be included in the output.
		 *
		 * @param file the file path to check
		 *
		 * @return true if the file should be included in the output
		 */
		virtual bool fileIsIncluded(const std::string &file) = 0;

		/**
		 * Returns if a file/line pair contains executable code.
		 *
		 * @param file the filename
		 * @param lineNr the line number in the file
		 *
		 * @return true if this is executable code, false otherwise
		 */
		virtual bool lineIsCode(const std::string &file, unsigned int lineNr) = 0;

		virtual LineExecutionCount getLineExecutionCount(const std::string &file, unsigned int lineNr) = 0;

		virtual ExecutionSummary getExecutionSummary() = 0;

		/**
		 * Marshal collected data to a data chunk.
		 *
		 * @param szOut the size of the data (return parameter)
		 *
		 * @return the data, or NULL if something failed
		 */
		virtual void *marshal(size_t *szOut) = 0;

		/**
		 * Unmarshal marshalled data in-place.
		 *
		 * @param data the data to unmarshal
		 * @param sz the size of the data
		 *
		 * @return true if the data could be unmarshalled, false otherwise
		 */
		virtual bool unMarshal(void *data, size_t sz) = 0;

		static IReporter &create(IFileParser &elf, ICollector &collector, IFilter &filter);
	};
}
