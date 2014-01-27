#include <reporter.hh>
#include <elf.hh>
#include <configuration.hh>
#include <writer.hh>
#include <utils.hh>
#include <output-handler.hh>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string>
#include <list>
#include <unordered_map>

#include "writer-base.hh"

using namespace kcov;


static const uint8_t icon_ruby[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f,
0x18, 0x10, 0x5d, 0x57, 0x34, 0x6e, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b,
0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d,
0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45,
0xff, 0x35, 0x2f, 0x00, 0x00, 0x00, 0xd0, 0x33, 0x9a, 0x9d, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41,
0x54, 0x78, 0xda, 0x63, 0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00,
0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

static const uint8_t icon_amber[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f,
0x28, 0x04, 0x98, 0xcb, 0xd6, 0xe0, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b,
0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d,
0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45,
0xff, 0xe0, 0x50, 0x00, 0x00, 0x00, 0xa2, 0x7a, 0xda, 0x7e, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41,
0x54, 0x78, 0xda, 0x63, 0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00,
0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

static const uint8_t icon_emerald[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x25,
0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f, 0x22, 0x2b,
0xc9, 0xf5, 0x03, 0x33, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b, 0x12, 0x00, 0x00,
0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d, 0x41, 0x00, 0x00, 0xb1,
0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45, 0x1b, 0xea, 0x59, 0x0a, 0x0a,
0x0a, 0x0f, 0xba, 0x50, 0x83, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0x60, 0x00,
0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
0x42, 0x60, 0x82 };

static const uint8_t icon_snow[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f,
0x1e, 0x1d, 0x75, 0xbc, 0xef, 0x55, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b,
0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d,
0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45,
0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x55, 0xc2, 0xd3, 0x7e, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41,
0x54, 0x78, 0xda, 0x63, 0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00,
0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

static const uint8_t icon_glass[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d, 0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b,
0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
0x55, 0xc2, 0xd3, 0x7e, 0x00, 0x00, 0x00, 0x01, 0x74, 0x52, 0x4e, 0x53, 0x00, 0x40, 0xe6, 0xd8, 0x66,
0x00, 0x00, 0x00, 0x01, 0x62, 0x4b, 0x47, 0x44, 0x00, 0x88, 0x05, 0x1d, 0x48, 0x00, 0x00, 0x00, 0x09,
0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b, 0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc,
0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x13, 0x0f, 0x08, 0x19, 0xc4, 0x40,
0x56, 0x10, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x60, 0x00, 0x00, 0x00,
0x02, 0x00, 0x01, 0x48, 0xaf, 0xa4, 0x71, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
0x60, 0x82 };

static const char css_text[] = "/* Based upon the lcov CSS style, style files can be reused */\n"
		"body { color: #000000; background-color: #FFFFFF; }\n"
		"a:link { color: #284FA8; text-decoration: underline; }\n"
		"a:visited { color: #00CB40; text-decoration: underline; }\n"
		"a:active { color: #FF0040; text-decoration: underline; }\n"
		"td.title { text-align: center; padding-bottom: 10px; font-size: 20pt; font-weight: bold; }\n"
		"td.ruler { background-color: #6688D4; }\n"
		"td.headerItem { text-align: right; padding-right: 6px; font-family: sans-serif; font-weight: bold; }\n"
		"td.headerValue { text-align: left; color: #284FA8; font-family: sans-serif; font-weight: bold; }\n"
		"td.versionInfo { text-align: center; padding-top:  2px; }\n"
		"pre.source { font-family: monospace; white-space: pre; }\n"
		"span.lineNum { background-color: #EFE383; }\n"
		"span.lineCov { background-color: #CAD7FE; }\n"
		"span.linePartCov { background-color: #FFEA20; }\n"
		"span.lineNoCov { background-color: #FF6230; }\n"
		"td.tableHead { text-align: center; color: #FFFFFF; background-color: #6688D4; font-family: sans-serif; font-size: 120%; font-weight: bold; }\n"
		"td.coverFile { text-align: left; padding-left: 10px; padding-right: 20px; color: #284FA8; background-color: #DAE7FE; font-family: monospace; }\n"
		"td.coverBar { padding-left: 10px; padding-right: 10px; background-color: #DAE7FE; }\n"
		"td.coverBarOutline { background-color: #000000; }\n"
		"td.coverPerHi { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #A7FC9D; font-weight: bold; }\n"
		"td.coverPerLeftMed { text-align: left; padding-left: 10px; padding-right: 10px; background-color: #FFEA20; font-weight: bold; }\n"
		"td.coverPerLeftLo { text-align: left; padding-left: 10px; padding-right: 10px; background-color: #FF0000; font-weight: bold; }\n"
		"td.coverPerLeftHi { text-align: left; padding-left: 10px; padding-right: 10px; background-color: #A7FC9D; font-weight: bold; }\n"
		"td.coverNumHi { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #A7FC9D; }\n"
		"td.coverPerMed { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FFEA20; font-weight: bold; }\n"
		"td.coverNumMed { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FFEA20; }\n"
		"td.coverPerLo { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FF0000; font-weight: bold; }\n"
		"td.coverNumLo { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FF0000; }\n";


class HtmlWriter : public WriterBase
{
public:
	HtmlWriter(IElf &elf, IReporter &reporter, IOutputHandler &output) :
		WriterBase(elf, reporter, output)
	{
		m_indexDirectory = output.getBaseDirectory();
		m_outDirectory = output.getOutDirectory();
		m_summaryDbFileName = m_outDirectory + "summary.db";
	}

	void onStop()
	{
	}

private:

	void writeOne(File *file)
	{
		enum IConfiguration::OutputType type = IConfiguration::getInstance().getOutputType();
		std::string outName = m_outDirectory + "/" + file->m_outFileName;
		uint64_t nTotalExecutedAddresses = 0;
		unsigned int nExecutedLines = 0;
		unsigned int nCodeLines = 0;

		std::string s =
				"<pre class=\"source\">\n";
		if (type == IConfiguration::OUTPUT_PROFILER) {
			for (unsigned int n = 1; n < file->m_lastLineNr; n++) {
				IReporter::LineExecutionCount cnt =
						m_reporter.getLineExecutionCount(file->m_name.c_str(), n);

				nTotalExecutedAddresses += cnt.m_hits;
			}
		}

		for (unsigned int n = 1; n < file->m_lastLineNr; n++) {
			std::string line = file->m_lineMap[n];

			s += "<span class=\"lineNum\">" + fmt("%5u", n) + "</span>";

			if (!m_reporter.lineIsCode(file->m_name.c_str(), n)) {
				if (type == IConfiguration::OUTPUT_COVERAGE)
					s += "              : " + escapeHtml(line) + "</span>\n";
				else
					s += "                : " + escapeHtml(line) + "</span>\n";
				continue;
			}

			IReporter::LineExecutionCount cnt =
					m_reporter.getLineExecutionCount(file->m_name.c_str(), n);

			std::string lineClass = "lineNoCov";

			if (type == IConfiguration::OUTPUT_COVERAGE)
				nExecutedLines += !!cnt.m_hits;
			else
				nExecutedLines += cnt.m_hits;
			nCodeLines++;

			if (cnt.m_hits == cnt.m_possibleHits)
				lineClass = "lineCov";
			else if (cnt.m_hits)
				lineClass = "linePartCov";

			if (type == IConfiguration::OUTPUT_COVERAGE) {
				s += "<span class=\"" + lineClass + "\">    " +
						fmt("%3u", cnt.m_hits) + "  / " + fmt("%3u", cnt.m_possibleHits) + ": " +
						escapeHtml(line) +
						"</span>\n";
			} else {
				double percentage = 0;

				if (nTotalExecutedAddresses != 0)
					percentage = ((double)cnt.m_hits / (double)nTotalExecutedAddresses) * 100;

				unsigned int red = 21;
				unsigned int green = 129;
				unsigned int blue = 148;

				if (percentage > 0 && percentage < 5) {
					red = 71; green = 157; blue = 117;
				} else if (percentage >= 5 && percentage < 15) {
					red = 123; green = 195; blue = 73;
				} else if (percentage >= 15 && percentage < 30) {
					red = 209; green = 214; blue = 0;
				} else if (percentage >= 30 && percentage < 45) {
					red = 240; green = 151; blue = 0;
				} else if (percentage >= 45) {
					red = 252; green = 107; blue = 0;
				}

				s += "<span style=\"background-color:" + fmt("#%02x%02x%02x", red, green, blue) + "\">" +
						fmt("%8u", cnt.m_hits) + " / " + fmt("%3u%%", (unsigned int)percentage) + " </span>: " +
						"<span>" +
						escapeHtml(line) +
						"</span>\n";
			}
		}
		s += "</pre>\n";

		s = getHeader(nCodeLines, nExecutedLines) + s + getFooter(file);

		write_file((void *)s.c_str(), s.size(), outName.c_str());

		// Update the execution count
		file->m_executedLines = nExecutedLines;
		file->m_codeLines = nCodeLines;
	}

	void writeIndex()
	{
		enum IConfiguration::OutputType type = IConfiguration::getInstance().getOutputType();
		unsigned int nTotalExecutedLines = 0;
		unsigned int nTotalCodeLines = 0;
		uint64_t nTotalExecutedAddresses = 0;
		std::string s;

		std::multimap<double, std::string> fileListByPercent;
		std::multimap<double, std::string> fileListByUncoveredLines;
		std::multimap<double, std::string> fileListByFileLength;
		std::map<std::string, std::string> fileListByName;

		if (type == IConfiguration::OUTPUT_PROFILER) {
			for (const auto &it : m_files) {
				auto file = it.second;

				nTotalExecutedAddresses += file->m_executedLines;
			}
		}
		for (const auto &it : m_files) {
			auto file = it.second;
			unsigned int nExecutedLines = file->m_executedLines;
			unsigned int nCodeLines = file->m_codeLines;
			double percent = 0;

			if (type == IConfiguration::OUTPUT_COVERAGE) {
				if (nCodeLines != 0)
					percent = ((double)nExecutedLines / (double)nCodeLines) * 100;

				nTotalCodeLines += nCodeLines;
				nTotalExecutedLines += nExecutedLines;
			} else {
				if (nExecutedLines == 0)
					continue;

				// Profiler
				percent = ((double)nExecutedLines / (double)nTotalExecutedAddresses) * 100;
			}

			std::string coverPer = strFromPercentage(percent);

			std::string listName = file->m_name;

			size_t pos = listName.find(m_commonPath);
			unsigned int stripLevel = IConfiguration::getInstance().getPathStripLevel();

			if (pos != std::string::npos && m_commonPath.size() != 0 && stripLevel != ~0U) {
				std::string pathToRemove = m_commonPath;

				for (unsigned int i = 0; i < stripLevel; i++) {
					size_t slashPos = pathToRemove.rfind("/");

					if (slashPos == std::string::npos)
						break;
					pathToRemove = pathToRemove.substr(0, slashPos);
				}

				std::string prefix = "[...]";

				if (pathToRemove == "")
					prefix = "";
				listName = prefix + listName.substr(pathToRemove.size());
			}

			std::string cur =
				"    <tr>\n"
				"      <td class=\"coverFile\"><a href=\"" + file->m_outFileName + "\" title=\"" + file->m_fileName + "\">" + listName + "</a></td>\n"
				"      <td class=\"coverBar\" align=\"center\">\n"
				"        <table border=\"0\" cellspacing=\"0\" cellpadding=\"1\"><tr><td class=\"coverBarOutline\">" + constructBar(percent) + "</td></tr></table>\n"
				"      </td>\n";
			if (type == IConfiguration::OUTPUT_COVERAGE)
				cur += "      <td class=\"coverPer" + coverPer + "\">" + fmt("%.1f", percent) + "&nbsp;%</td>\n"
				"      <td class=\"coverNum" + coverPer + "\">" + fmt("%u", nExecutedLines) + "&nbsp;/&nbsp;" + fmt("%u", nCodeLines) + "&nbsp;lines</td>\n"
				"    </tr>\n";
			else
				cur += "      <td class=\"coverPer" + coverPer + "\">" + fmt("%.1f", percent) + "&nbsp;%</td>\n"
				"    </tr>\n";

			fileListByName[file->m_name] = cur;
			fileListByPercent.insert(std::pair<double, std::string>(percent, cur));
			fileListByUncoveredLines.insert(std::pair<double, std::string>(nCodeLines - nExecutedLines, cur));
			fileListByFileLength.insert(std::pair<double, std::string>(nCodeLines, cur));
		}

		if (IConfiguration::getInstance().getSortType() == IConfiguration::PERCENTAGE) {
			for (const auto &it : fileListByPercent)
				s = s + it.second;
		}
		else if (IConfiguration::getInstance().getSortType() == IConfiguration::REVERSE_PERCENTAGE) {
			for (const auto &it : fileListByPercent)
				s = it.second + s;
		}
		else if (IConfiguration::getInstance().getSortType() == IConfiguration::UNCOVERED_LINES) {
			for (const auto &it : fileListByUncoveredLines)
				s = it.second + s; // Sort backwards
		}
		else if (IConfiguration::getInstance().getSortType() == IConfiguration::FILE_LENGTH) {
			for (const auto &it : fileListByFileLength)
				s = it.second + s; // Ditto
		}
		else {
			for (const auto &it : fileListByName)
				s = s + it.second;
		}

		s = getHeader(nTotalCodeLines, nTotalExecutedLines) + getIndexHeader() + s + getFooter(NULL);

		write_file((void *)s.c_str(), s.size(), (m_outDirectory + "index.html").c_str());

		// Produce a summary
		IReporter::ExecutionSummary summary = m_reporter.getExecutionSummary();
		size_t sz;

		void *data = marshalSummary(summary,
				IConfiguration::getInstance().getBinaryName(), &sz);

		if (data)
			write_file(data, sz, "%s", m_summaryDbFileName.c_str());

		free(data);
	}

	void writeGlobalIndex()
	{
		DIR *dir;
		struct dirent *de;
		std::string idx = m_indexDirectory.c_str();
		std::string s;
		unsigned int nTotalExecutedLines = 0;
		unsigned int nTotalCodeLines = 0;

		dir = opendir(idx.c_str());
		panic_if(!dir, "Can't open directory %s\n", idx.c_str());

		for (de = readdir(dir); de; de = readdir(dir)) {
			std::string cur = idx + de->d_name + "/summary.db";

			if (!file_exists(cur.c_str()))
				continue;

			size_t sz;
			void *data = read_file(&sz, "%s", cur.c_str());

			if (!data)
				continue;

			IReporter::ExecutionSummary summary;
			std::string name;
			bool res = unMarshalSummary(data, sz, summary, name);
			free(data);

			if (!res)
				continue;


			double percent = 0;

			if (summary.m_lines != 0)
				percent = (summary.m_executedLines / (double)summary.m_lines) * 100;
			nTotalCodeLines += summary.m_lines;
			nTotalExecutedLines += summary.m_executedLines;

			std::string coverPer = strFromPercentage(percent);

			s +=
					"    <tr>\n"
					"      <td class=\"coverFile\"><a href=\"" + std::string(de->d_name) + "/index.html\" title=\"" + name + "\">" + name + "</a></td>\n"
					"      <td class=\"coverBar\" align=\"center\">\n"
					"        <table border=\"0\" cellspacing=\"0\" cellpadding=\"1\"><tr><td class=\"coverBarOutline\">" + constructBar(percent) + "</td></tr></table>\n"
					"      </td>\n"
					"      <td class=\"coverPer" + coverPer + "\">" + fmt("%.1f", percent) + "&nbsp;%</td>\n"
					"      <td class=\"coverNum" + coverPer + "\">" + fmt("%u", summary.m_executedLines) + "&nbsp;/&nbsp;" + fmt("%u", summary.m_lines) + "&nbsp;lines</td>\n"
					"    </tr>\n";

		}
		s = getHeader(nTotalCodeLines, nTotalExecutedLines, false) + getIndexHeader() + s + getFooter(NULL);

		write_file((void *)s.c_str(), s.size(), (m_indexDirectory + "index.html").c_str());
		closedir(dir);
	}


	void write()
	{
		for (const auto &it : m_files)
			writeOne(it.second);

		setupCommonPaths();

		writeIndex();

		writeGlobalIndex();
	}

	std::string strFromPercentage(double percent)
	{
		IConfiguration &conf = IConfiguration::getInstance();
		std::string coverPer = "Med";

		if (percent >= conf.getHighLimit())
			coverPer = "Hi";
		else if (percent < conf.getLowLimit())
			coverPer = "Lo";

		return coverPer;
	}

	std::string getIndexHeader()
	{
		return std::string(
				"<center>\n"
				"  <table width=\"80%\" cellpadding=\"2\" cellspacing=\"1\" border=\"0\">\n"
				"    <tr>\n"
				"      <td width=\"50%\"><br/></td>\n"
				"      <td width=\"15%\"></td>\n"
				"      <td width=\"15%\"></td>\n"
				"      <td width=\"20%\"></td>\n"
				"    </tr>\n"
				"    <tr>\n"
				"      <td class=\"tableHead\">Filename</td>\n"
				"      <td class=\"tableHead\" colspan=\"3\">Coverage</td>\n"
				"    </tr>\n"
		);
	}

	std::string getFooter(File *file)
	{
		return std::string(
				"<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
				"  <tr><td class=\"ruler\"><img src=\"glass.png\" width=\"3\" height=\"3\" alt=\"\"/></td></tr>\n"
				"  <tr><td class=\"versionInfo\">Generated by: <a href=\"http://simonkagstrom.github.com/kcov/index.html\">Kcov</a></td></tr>\n"
				"</table>\n"
				"<br/>\n"
				"</body>\n"
				"</html>\n"
				);
	}

	std::string getHeader(unsigned int nCodeLines, unsigned int nExecutedLines,
			bool includeCommand = true)
	{
		IConfiguration &conf = IConfiguration::getInstance();
		std::string percentage_text = "Lo";
		double percentage = 0;
		char date_buf[80];
		time_t t;
		struct tm *tm;

		if (nCodeLines != 0)
			percentage = ((double)nExecutedLines / (double)nCodeLines) * 100;

		if (percentage > conf.getLowLimit() && percentage < conf.getHighLimit())
			percentage_text = "Med";
		else if (percentage >= conf.getHighLimit())
			percentage_text = "Hi";

		t = time(NULL);
		tm = localtime(&t);
		strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", tm);

		std::string date(date_buf);
		std::string instrumentedLines = fmt("%u", nCodeLines);
		std::string lines = fmt("%u", nExecutedLines);
		std::string covered = fmt("%.1f%%", percentage);

		if (conf.getOutputType() == IConfiguration::OUTPUT_PROFILER)
			covered = fmt("%u samples", nExecutedLines);

		std::string commandStr = "";
		if (includeCommand)
			commandStr = "          <td class=\"headerItem\" width=\"20%\">Command:</td>\n"
					"          <td class=\"headerValue\" width=\"80%\" colspan=6>" + escapeHtml(conf.getBinaryName()) + "</td>\n";

		return std::string(
				"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
				"<html>\n"
				"<head>\n"
				"  <title>Coverage - " + escapeHtml(conf.getBinaryName()) + "</title>\n"
				"  <link rel=\"stylesheet\" type=\"text/css\" href=\"bcov.css\"/>\n"
				"</head>\n"
				"<body>\n"
				"<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
				"  <tr><td class=\"title\">Coverage Report</td></tr>\n"
				"  <tr><td class=\"ruler\"><img src=\"glass.png\" width=\"3\" height=\"3\" alt=\"\"/></td></tr>\n"
				"  <tr>\n"
				"    <td width=\"100%\">\n"
				"      <table cellpadding=\"1\" border=\"0\" width=\"100%\">\n"
				"        <tr>\n" +
				commandStr +
				"        </tr>\n"
				"        <tr>\n"
				"          <td class=\"headerItem\" width=\"20%\">Date:</td>\n"
				"          <td class=\"headerValue\" width=\"15%\">" + escapeHtml(date) + "</td>\n"
				"          <td width=\"5%\"></td>\n"
				"          <td class=\"headerItem\" width=\"20%\">Instrumented&nbsp;lines:</td>\n"
				"          <td class=\"headerValue\" width=\"10%\">" + instrumentedLines + "</td>\n"
				"        </tr>\n"
				"        <tr>\n"
				"          <td class=\"headerItem\" width=\"20%\">Code&nbsp;covered:</td>\n"
				"          <td class=\"coverPerLeft" + percentage_text + "\" width=\"15%\">" + covered + "</td>\n"
				"          <td width=\"5%\"></td>\n"
				"          <td class=\"headerItem\" width=\"20%\">Executed&nbsp;lines:</td>\n"
				"          <td class=\"headerValue\" width=\"10%\">" + lines + "</td>\n"
				"        </tr>\n"
				"      </table>\n"
				"    </td>\n"
				"  </tr>\n"
				"  <tr><td class=\"ruler\"><img src=\"glass.png\" width=\"3\" height=\"3\" alt=\"\"/></td></tr>\n"
				"</table>\n"
				);
	}

	std::string constructBar(double percent)
	{
		const char* color = "ruby.png";
		char buf[1024];
		int width = (int)(percent+0.5);
		IConfiguration &conf = IConfiguration::getInstance();

		if (percent >= conf.getHighLimit())
			color = "emerald.png";
		else if (percent > conf.getLowLimit())
			color = "amber.png";
		else if (percent <= 1)
			color = "snow.png";

		xsnprintf(buf, sizeof(buf),
				"<img src=\"%s\" width=\"%d\" height=\"10\" alt=\"%.1f%%\"/><img src=\"snow.png\" width=\"%d\" height=\"10\" alt=\"%.1f%%\"/>",
				color, width, percent, 100 - width, percent);

		return std::string(buf);
	}

	char *escapeHelper(char *dst, const char *what)
	{
		int len = strlen(what);

		strcpy(dst, what);

		return dst + len;
	}

	std::string escapeHtml(std::string &str)
	{
		const char *s = str.c_str();
		char buf[4096];
		char *dst = buf;
		size_t len = strlen(s);
		size_t i;

		memset(buf, 0, sizeof(buf));
		for (i = 0; i < len; i++) {
			char c = s[i];

			switch (c) {
			case '<':
				dst = escapeHelper(dst, "&lt;");
				break;
			case '>':
				dst = escapeHelper(dst, "&gt;");
				break;
			case '&':
				dst = escapeHelper(dst, "&amp;");
				break;
			case '\"':
				dst = escapeHelper(dst, "&quot;");
				break;
			case '\'':
				dst = escapeHelper(dst, "&#039;");
				break;
			case '/':
				dst = escapeHelper(dst, "&#047;");
				break;
			case '\\':
				dst = escapeHelper(dst, "&#092;");
				break;
			case '\n': case '\r':
				dst = escapeHelper(dst, " ");
				break;
			default:
				*dst = c;
				dst++;
				break;
			}
		}

		return std::string(buf);
	}

	void writeHelperFiles(std::string dir)
	{
		write_file(icon_ruby, sizeof(icon_ruby), "%s/ruby.png", dir.c_str());
		write_file(icon_amber, sizeof(icon_amber), "%s/amber.png", dir.c_str());
		write_file(icon_emerald, sizeof(icon_emerald), "%s/emerald.png", dir.c_str());
		write_file(icon_snow, sizeof(icon_snow), "%s/snow.png", dir.c_str());
		write_file(icon_glass, sizeof(icon_glass), "%s/glass.png", dir.c_str());
		write_file(css_text, sizeof(css_text), "%s/bcov.css", dir.c_str());
	}

	void onStartup()
	{
		writeHelperFiles(m_indexDirectory);
		writeHelperFiles(m_outDirectory);
	}


	std::string m_outDirectory;
	std::string m_indexDirectory;
	std::string m_summaryDbFileName;
};

namespace kcov
{
	IWriter &createHtmlWriter(IElf &elf, IReporter &reporter, IOutputHandler &output)
	{
		return *new HtmlWriter(elf, reporter, output);
	}
}
