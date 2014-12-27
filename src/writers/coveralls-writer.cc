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
		WriterBase(parser, reporter),
		m_outFile("/tmp/coveralls.out") // FIXME! Temporary
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

		out << "{\n";
		out << " \'service_name\': \'travis-ci\',\n";
		out << " \'service_job_id\': \'12345678\',\n"; // FIXME!
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
	std::string m_outFile;
};

namespace kcov
{
	IWriter &createCoverallsWriter(IFileParser &parser, IReporter &reporter)
	{
		return *new CoverallsWriter(parser, reporter);
	}
}
