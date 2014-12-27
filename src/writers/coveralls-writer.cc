/*
 * Writer for the "coveralls.io" web service.
 */
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

class CoverallsWriter : public WriterBase
{
public:
	CoverallsWriter(IFileParser &parser, IReporter &reporter) :
		WriterBase(parser, reporter)
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
		IConfiguration &conf = IConfiguration::getInstance();
		const std::string &repo_token = conf.getCoverallsRepoToken();

		// No token? Skip output then
		if (repo_token == "")
			return;

		std::string outFile = conf.getOutDirectory() + "/" + conf.getBinaryName() + "/coveralls.out";

		std::ofstream out(outFile);

		// Output directory not writable?
		if (!out.is_open())
			return;

		out << "{\n";
		out << " \'repo_token\': \'" + repo_token + "\',\n"; // FIXME!
		out << " \'source_files\': [\n";
		setupCommonPaths();

		for (FileMap_t::const_iterator it = m_files.begin();
				it != m_files.end();
				++it) {
			File *file = it->second;

			out << "  {\n";
			out << "   \'name\': \'" + escape_json(file->m_fileName) + "\',\n";
			out << "   \'source\': \'";

			// Output source lines
			for (unsigned int n = 1; n < file->m_lastLineNr; n++)
				out << escape_json(file->m_lineMap[n]) << "\\n";
			out << "\',\n";
			out << "   \'coverage\': [";

			// And coverage
			for (unsigned int n = 1; n < file->m_lastLineNr; n++) {
				if (!m_reporter.lineIsCode(file->m_name, n)) {
					out << "null";
				} else {
					IReporter::LineExecutionCount cnt =
							m_reporter.getLineExecutionCount(file->m_name, n);

					out << cnt.m_hits;
				}

				if (n != file->m_lastLineNr - 1)
					out << ",";
			}
			out << "   ],\n";
			out << "  },\n";
		}
		out << " ]\n";
		out << "}\n";
	}

private:
};

namespace kcov
{
	IWriter &createCoverallsWriter(IFileParser &parser, IReporter &reporter)
	{
		return *new CoverallsWriter(parser, reporter);
	}
}
