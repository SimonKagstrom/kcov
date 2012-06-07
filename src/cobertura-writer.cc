#include <reporter.hh>
#include <elf.hh>
#include <configuration.hh>
#include <writer.hh>
#include <filter.hh>
#include <utils.hh>
#include <output-handler.hh>

#include <string>
#include <list>
#include <unordered_map>

#include "writer-base.hh"

using namespace kcov;

class CoberturaWriter : public WriterBase, public IWriter
{
public:
	CoberturaWriter(IElf &elf, IReporter &reporter, IOutputHandler &output) :
		WriterBase(elf, reporter, output),
		m_filter(IFilter::getInstance()),
		m_output(output)
	{
	}

	void onStartup()
	{
	}

	void onStop()
	{
	}

	void write()
	{
		std::string str;

		unsigned int nTotalExecutedLines = 0;
		unsigned int nTotalCodeLines = 0;

		m_fileMutex.lock();
		for (FileMap_t::iterator it = m_files.begin();
				it != m_files.end();
				it++) {
			File *file = it->second;

			if (m_filter.runFilters(file->m_name) == false)
				continue;

			str = str + writeOne(file);
			nTotalCodeLines += file->m_codeLines;
			nTotalExecutedLines += file->m_executedLines;
		}
		m_fileMutex.unlock();

		str = getHeader(nTotalCodeLines, nTotalExecutedLines) + str + getFooter();

		write_file((void *)str.c_str(), str.size(),
				(m_output.getOutDirectory() + "cobertura.xml").c_str());
	}

private:
	std::string mangleFileName(std::string name)
	{
		std::string out = name;

		for (size_t pos = 0;
				pos < name.size();
				pos++)
		{
			if (out[pos] == '.' ||
					out[pos] == '-')
				out[pos] = '_';
		}

		return out;
	}

	std::string writeOne(File *file)
	{
		std::string out;

		std::string mangledName = mangleFileName(file->m_fileName);

		unsigned int nExecutedLines = 0;
		unsigned int nCodeLines = 0;

		for (unsigned int n = 1; n < file->m_lastLineNr; n++) {
			std::string line = file->m_lineMap[n];

			if (!m_reporter.lineIsCode(file->m_name.c_str(), n))
					continue;

			IReporter::LineExecutionCount cnt =
					m_reporter.getLineExecutionCount(file->m_name.c_str(), n);

			nExecutedLines += !!cnt.m_hits;
			nCodeLines++;

			out = out +
					"						<line number=\"" + fmt("%u", n) +
					"\" hits=\"" + fmt("%u", cnt.m_hits) +
					"\" branch=\"false\"/>\n";

			// Update the execution count
			file->m_executedLines = nExecutedLines;
			file->m_codeLines = nCodeLines;
		}

		if (nCodeLines == 0)
			nCodeLines = 1;

		out = "				<class name=\"" + mangledName + "\" filename=\"" +
				file->m_name + "\" line-rate=\"" +
				fmt("%.3f", nExecutedLines / (float)nCodeLines) +
				"\" branch-rate=\"1.0\" complexity=\"1.0\">\n" +
				"					<lines>\n" +
				out +
				"					</lines>\n"
				"				</class>\n";

		return out;
	}

	std::string getHeader(unsigned int nCodeLines, unsigned int nExecutedLines)
	{
		time_t t;
		struct tm *tm;
		char date_buf[80];

		t = time(NULL);
		tm = localtime(&t);
		strftime(date_buf, sizeof(date_buf), "%s", tm);

		if (nCodeLines == 0)
			nCodeLines = 1;

		std::string lineRate = fmt("%.3f", nExecutedLines / (float)nCodeLines);

		return
				"<?xml version=\"1.0\" ?>\n"
				"<!DOCTYPE coverage SYSTEM 'http://cobertura.sourceforge.net/xml/coverage-03.dtd'>\n"
				"<coverage line-rate=\"" + lineRate + "\" version=\"1.9\" timestamp=\"" + std::string(date_buf) + "\">\n"
				"	<sources>\n"
				"		<source>.</source>\n"
				"	</sources>\n"
				"	<packages>\n"
				"		<package name=\"\" line-rate=\"" + lineRate + "\" branch-rate=\"1.0\" complexity=\"1.0\">\n"
				"			<classes>\n"
				;
	}

	std::string getFooter()
	{
		return
				"			</classes>\n"
				"		</package>\n"
				"	</packages>\n"
				"</coverage>\n";
	}


	IFilter &m_filter;
	IOutputHandler &m_output;
};

namespace kcov
{
	IWriter &createCoberturaWriter(IElf &elf, IReporter &reporter, IOutputHandler &output)
	{
		return *new CoberturaWriter(elf, reporter, output);
	}
}
