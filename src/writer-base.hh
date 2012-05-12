#pragma once

#include <writer.hh>
#include <reporter.hh>
#include <elf.hh>

#include <string>
#include <unordered_map>

namespace kcov
{
	class IElf;
	class IReporter;
	class IOutputHandler;

	class WriterBase : public IElf::IListener
	{
	protected:
		WriterBase(IElf &elf, IReporter &reporter, IOutputHandler &output);

		class File
		{
		public:
			typedef std::unordered_map<unsigned int, std::string> LineMap_t;

			File(const char *filename);

			std::string m_name;
			std::string m_fileName;
			std::string m_outFileName;
			LineMap_t m_lineMap;
			unsigned int m_codeLines;
			unsigned int m_executedLines;
			unsigned int m_lastLineNr;

		private:
			void readFile(const char *filename);
		};

		typedef std::unordered_map<std::string, File *> FileMap_t;


		/* Called when the ELF is parsed */
		void onLine(const char *file, unsigned int lineNr, unsigned long addr);


		void *marshalSummary(IReporter::ExecutionSummary &summary,
				std::string &name, size_t *sz);

		bool unMarshalSummary(void *data, size_t sz,
				IReporter::ExecutionSummary &summary,
				std::string &name);

		IElf &m_elf;
		IReporter &m_reporter;
		FileMap_t m_files;
	};
}
