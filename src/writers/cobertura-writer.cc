namespace std
{
class type_info;
}

#include <reporter.hh>
#include <file-parser.hh>
#include <configuration.hh>
#include <writer.hh>
#include <utils.hh>

#include <string>
#include <list>
#include <unordered_map>
#include <iostream>
#include <fstream>

#include "writer-base.hh"

using namespace kcov;

class CoberturaWriter : public WriterBase
{
public:
	CoberturaWriter(IFileParser &parser, IReporter &reporter, const std::string &outFile) :
			WriterBase(parser, reporter), m_outFile(outFile), m_maxPossibleHits(parser.maxPossibleHits())
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
		std::ofstream out(m_outFile);

		// Output directory not writable?
		if (!out.is_open())
			return;

		unsigned int nTotalExecutedLines = 0;
		unsigned int nTotalCodeLines = 0;

		setupCommonPaths();

		for (FileMap_t::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
		{
			File *file = it->second;

			nTotalCodeLines += file->m_codeLines;
			nTotalExecutedLines += file->m_executedLines;
		}
		out << getHeader(nTotalCodeLines, nTotalExecutedLines);

		for (FileMap_t::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
		{
			File *file = it->second;

			out << writeOne(file);
		}

		out << getFooter();
	}

private:
	const std::string mangleFileName(const std::string &name)
	{
		std::string out = name;

		for (size_t pos = 0; pos < name.size(); pos++)
		{
			if (out[pos] == '.' || out[pos] == '-')
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

		for (unsigned int n = 1; n < file->m_lastLineNr; n++)
		{
			if (!m_reporter.lineIsCode(file->m_name, n))
				continue;

			IReporter::LineExecutionCount cnt = m_reporter.getLineExecutionCount(file->m_name, n);

			nExecutedLines += !!cnt.m_hits;
			nCodeLines++;

			unsigned int hits = cnt.m_hits;

			if (hits && m_maxPossibleHits == IFileParser::HITS_SINGLE)
				hits = 1;

			out = out + "						<line number=\"" + fmt("%u", n) + "\" hits=\"" + fmt("%u", hits) + "\"/>\n";

			// Update the execution count
			file->m_executedLines = nExecutedLines;
			file->m_codeLines = nCodeLines;
		}

		if (nCodeLines == 0)
			nCodeLines = 1;

		std::string filename = file->m_name;
		size_t pos = filename.find(m_commonPath);

		if (pos != std::string::npos && filename.size() > m_commonPath.size())
			filename = filename.substr(m_commonPath.size() + 1);

		out = "				<class name=\"" + mangledName + "\" filename=\"" + filename + "\" line-rate=\""
				+ fmt("%.3f", nExecutedLines / (float) nCodeLines) + "\">\n" + "					<lines>\n" + out + "					</lines>\n"
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

		std::string linesCovered = fmt("%u", nExecutedLines);
		std::string linesValid = fmt("%u", nCodeLines);

		if (nCodeLines == 0)
			nCodeLines = 1;

		std::string lineRate = fmt("%.3f", nExecutedLines / (float) nCodeLines);

		return "<?xml version=\"1.0\" ?>\n"
				"<!DOCTYPE coverage SYSTEM 'http://cobertura.sourceforge.net/xml/coverage-04.dtd'>\n"
				"<coverage line-rate=\"" + lineRate + "\" version=\"1.9\" timestamp=\"" + std::string(date_buf) + "\">\n"
				"	<sources>\n"
				"		<source>" + m_commonPath + "/</source>\n"
				"	</sources>\n"
				"	<packages>\n"
				"		<package name=\"" + mangleFileName(IConfiguration::getInstance().keyAsString("command-name"))
				+ "\" line-rate=\"" + lineRate + "\" lines-covered=\"" + linesCovered + "\" lines-valid=\"" + linesValid + "\"branch-rate=\"1.0\" complexity=\"1.0\">\n"
						"			<classes>\n";
	}

	const std::string getFooter()
	{
		return "			</classes>\n"
				"		</package>\n"
				"	</packages>\n"
				"</coverage>\n";
	}

	std::string m_outFile;
	IFileParser::PossibleHits m_maxPossibleHits;
};

namespace kcov
{
	IWriter &createCoberturaWriter(IFileParser &parser, IReporter &reporter, const std::string &outFile)
	{
		return *new CoberturaWriter(parser, reporter, outFile);
	}
}
