#include <reporter.hh>
#include <file-parser.hh>
#include <configuration.hh>
#include <writer.hh>
#include <utils.hh>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string>
#include <list>
#include <unordered_map>

#include "writer-base.hh"

using namespace kcov;

// Generated
extern uint8_t css_text_data[];
extern uint8_t icon_amber_data[];
extern uint8_t icon_glass_data[];
extern uint8_t icon_emerald_data[];
extern uint8_t icon_ruby_data[];
extern uint8_t icon_snow_data[];

extern size_t css_text_data_size;
extern size_t icon_amber_data_size;
extern size_t icon_glass_data_size;
extern size_t icon_emerald_data_size;
extern size_t icon_ruby_data_size;
extern size_t icon_snow_data_size;


class HtmlWriter : public WriterBase
{
public:
	HtmlWriter(IFileParser &parser, IReporter &reporter,
			const std::string &indexDirectory,
			const std::string &outDirectory,
			const std::string &name,
			bool includeInTotals) :
		WriterBase(parser, reporter),
		m_outDirectory(outDirectory + "/"),
		m_indexDirectory(indexDirectory + "/"),
		m_summaryDbFileName(outDirectory + "/summary.db"),
		m_name(name),
		m_includeInTotals(includeInTotals)
	{
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

			if (!m_reporter.lineIsCode(file->m_name, n)) {
				if (type == IConfiguration::OUTPUT_COVERAGE)
					s += "              : " + escape_html(line) + "</span>\n";
				else
					s += "                : " + escape_html(line) + "</span>\n";
				continue;
			}

			IReporter::LineExecutionCount cnt =
					m_reporter.getLineExecutionCount(file->m_name, n);

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
						escape_html(line) +
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
						escape_html(line) +
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

		s = getHeader(nTotalCodeLines, nTotalExecutedLines) + getIndexHeader() + s + getFooter(nullptr);

		write_file((void *)s.c_str(), s.size(), (m_outDirectory + "index.html").c_str());

		// Produce a summary
		IReporter::ExecutionSummary summary = m_reporter.getExecutionSummary();
		summary.m_includeInTotals = m_includeInTotals;
		size_t sz;

		void *data = marshalSummary(summary,
				m_name, &sz);

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
			// Skip entries (merged ones) that shouldn't be included in the totals
			if (summary.m_includeInTotals) {
				nTotalCodeLines += summary.m_lines;
				nTotalExecutedLines += summary.m_executedLines;
			}

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
		s = getHeader(nTotalCodeLines, nTotalExecutedLines, false) + getIndexHeader() + s + getFooter(nullptr);

		write_file((void *)s.c_str(), s.size(), (m_indexDirectory + "index.html").c_str());
		closedir(dir);
	}


	void write()
	{
		for (const auto &it : m_files)
			writeOne(it.second);

		setupCommonPaths();

		writeIndex();

		if (m_includeInTotals)
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

		t = time(nullptr);
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
					"          <td class=\"headerValue\" width=\"80%\" colspan=6>" + escape_html(conf.getBinaryName()) + "</td>\n";

		return std::string(
				"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
				"<html>\n"
				"<head>\n"
				"  <title>Coverage - " + escape_html(conf.getBinaryName()) + "</title>\n"
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
				"          <td class=\"headerValue\" width=\"15%\">" + escape_html(date) + "</td>\n"
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

	void writeHelperFiles(std::string dir)
	{
		write_file(icon_ruby_data, icon_ruby_data_size, "%s/ruby.png", dir.c_str());
		write_file(icon_amber_data, icon_amber_data_size, "%s/amber.png", dir.c_str());
		write_file(icon_emerald_data, icon_emerald_data_size, "%s/emerald.png", dir.c_str());
		write_file(icon_snow_data, icon_snow_data_size, "%s/snow.png", dir.c_str());
		write_file(icon_glass_data, icon_glass_data_size, "%s/glass.png", dir.c_str());
		write_file(css_text_data, css_text_data_size, "%s/bcov.css", dir.c_str());
	}

	void onStartup()
	{
		writeHelperFiles(m_indexDirectory);
		writeHelperFiles(m_outDirectory);
	}


	std::string m_outDirectory;
	std::string m_indexDirectory;
	std::string m_summaryDbFileName;
	std::string m_name;
	bool m_includeInTotals;
};

namespace kcov
{
	IWriter &createHtmlWriter(IFileParser &parser, IReporter &reporter,
			const std::string &indexDirectory,
			const std::string &outDirectory,
			const std::string &name,
			bool includeInTotals)
	{
		return *new HtmlWriter(parser, reporter, indexDirectory, outDirectory,
				name, includeInTotals);
	}
}
