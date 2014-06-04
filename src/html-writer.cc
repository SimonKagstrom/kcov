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
extern uint8_t source_file_text_data[];
extern uint8_t index_text_data[];
extern uint8_t tempo_text_data[];
extern uint8_t kcov_text_data[];
extern uint8_t jquery_text_data[];
extern uint8_t tablesorter_text_data[];

extern size_t css_text_data_size;
extern size_t icon_amber_data_size;
extern size_t icon_glass_data_size;
extern size_t icon_emerald_data_size;
extern size_t icon_ruby_data_size;
extern size_t icon_snow_data_size;
extern size_t source_file_text_data_size;
extern size_t index_text_data_size;
extern size_t tempo_text_data_size;
extern size_t kcov_text_data_size;
extern size_t jquery_text_data_size;
extern size_t tablesorter_text_data_size;


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
		auto &conf = IConfiguration::getInstance();
		std::string jsonOutName = m_outDirectory + "/" + file->m_jsonOutFileName;
		std::string htmlOutName = m_outDirectory + "/" + file->m_outFileName;
		unsigned int nExecutedLines = 0;
		unsigned int nCodeLines = 0;

		std::string json;

		// Produce each line in the file
		for (unsigned int n = 1; n < file->m_lastLineNr; n++) {
			const auto &line = file->m_lineMap[n];

			IReporter::LineExecutionCount cnt =
					m_reporter.getLineExecutionCount(file->m_name, n);

			std::string lineClass = "lineNoCov";

			nExecutedLines += !!cnt.m_hits;

			if (cnt.m_hits == cnt.m_possibleHits)
				lineClass = "lineCov";
			else if (cnt.m_hits)
				lineClass = "linePartCov";

			// Update the execution count
			file->m_executedLines = nExecutedLines;
			file->m_codeLines = nCodeLines;

			json += fmt(
					"{'lineNum':'%5u',"
					"'line':'%s'",
					n,
					escape_json(line).c_str()
					);

			if (m_reporter.lineIsCode(file->m_name, n)) {
				json += fmt(
					",'class':'%s',"
					"'coverage':'%3u / %3u  : ',",
					lineClass.c_str(),
					cnt.m_hits, cnt.m_possibleHits);

				nCodeLines++;
			} else {
				json += ",'coverage':'           : ',";
			}
			json += "},\n";

		}

		// Add the header
		json =  fmt(
				"var percent_low = %d;"
				"var percent_high = %d;"
				"\n"
				"var header = {"
				" 'command' : '%s',"
				" 'date' : '%s',"
				" 'instrumented' : %d,"
				" 'covered' : %d,"
				"};"
				"\n"
				"var data = [\n",
				conf.getLowLimit(),
				conf.getHighLimit(),
				escape_json(conf.getBinaryName()).c_str(),
				getDateNow().c_str(),
				nCodeLines,
				nExecutedLines
				) + json + "];";

		// Produce HTML outfile
		std::string html = fmt(
				"<script type=\"text/javascript\" src=\"%s\"></script>\n",
				file->m_jsonOutFileName.c_str());
		html += std::string((const char *)source_file_text_data, source_file_text_data_size);

		// .. And write both files to disk
		write_file((void *)json.c_str(), json.size(), jsonOutName.c_str());
		write_file((void *)html.c_str(), html.size(), htmlOutName.c_str());
	}

	void writeIndex()
	{
		auto &conf = IConfiguration::getInstance();
		unsigned int nTotalExecutedLines = 0;
		unsigned int nTotalCodeLines = 0;

		std::string json; // Not really json, but anyway

		for (const auto &it : m_files) {
			auto file = it.second;
			unsigned int nExecutedLines = file->m_executedLines;
			unsigned int nCodeLines = file->m_codeLines;
			double percent = 0;

			if (nCodeLines != 0)
				percent = ((double)nExecutedLines / (double)nCodeLines) * 100;

			nTotalCodeLines += nCodeLines;
			nTotalExecutedLines += nExecutedLines;

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

			int width = (int)(percent+0.5);

			json += fmt(
					"{'link':'%s',"
					"'title':'%s',"
					"'summary_name':'%s',"
					"'coverage_perc':'coverPer%s',"
					"'coverage_num':'coverNum%s',"
					"'covered_img':'%s',"
					"'covered_width':'%d',"
					"'uncovered_width':'%d',"
					"'covered':'%.1f',"
					"'covered_lines':'%d',"
					"'total_lines' : '%d'},\n",
					file->m_outFileName.c_str(),
					file->m_fileName.c_str(),
					listName.c_str(),
					strFromPercentage(percent).c_str(),
					strFromPercentage(percent).c_str(),
					imageFromPercent(percent).c_str(),
					width,
					100 - width,
					percent,
					nExecutedLines,
					nCodeLines
					);
		}


		// Add the header
		json =  fmt(
				"var percent_low = %d;"
				"var percent_high = %d;"
				"\n"
				"var header = {"
				" 'command' : '%s',"
				" 'date' : '%s',"
				" 'instrumented' : %d,"
				" 'covered' : %d,"
				"};"
				"\n"
				"var data = [\n",
				conf.getLowLimit(),
				conf.getHighLimit(),
				escape_json(conf.getBinaryName()).c_str(),
				getDateNow().c_str(),
				nTotalCodeLines,
				nTotalExecutedLines
				) + json + "];";

		// Produce HTML outfile
		auto html = std::string((const char *)index_text_data, index_text_data_size);

		// .. And write both files to disk
		write_file((void *)json.c_str(), json.size(), (m_outDirectory + "index.json").c_str());
		write_file((void *)html.c_str(), html.size(), (m_outDirectory + "index.html").c_str());

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
		auto &conf = IConfiguration::getInstance();
		DIR *dir;
		struct dirent *de;
		std::string idx = m_indexDirectory.c_str();
		std::string s;
		unsigned int nTotalExecutedLines = 0;
		unsigned int nTotalCodeLines = 0;

		dir = opendir(idx.c_str());
		panic_if(!dir, "Can't open directory %s\n", idx.c_str());

		std::string json;

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

			int width = (int)(percent+0.5);
			json += fmt(
					"{'link':'%s/index.html',"
					"'title':'%s',"
					"'summary_name':'%s',"
					"'coverage_perc':'coverPer%s',"
					"'coverage_num':'coverNum%s',"
					"'covered_img':'%s',"
					"'covered_width':'%d',"
					"'uncovered_width':'%d',"
					"'covered':'%.1f',"
					"'covered_lines':'%d',"
					"'total_lines' : '%d'},\n",
					de->d_name,
					name.c_str(),
					name.c_str(),
					strFromPercentage(percent).c_str(),
					strFromPercentage(percent).c_str(),
					imageFromPercent(percent).c_str(),
					width,
					100 - width,
					percent,
					summary.m_executedLines,
					summary.m_lines
			);
		}

		// Add the header
		json =  fmt(
				"var percent_low = %d;"
				"var percent_high = %d;"
				"\n"
				"var header = {"
				" 'command' : '%s',"
				" 'date' : '%s',"
				" 'instrumented' : %d,"
				" 'covered' : %d,"
				"};"
				"\n"
				"var data = [\n",
				conf.getLowLimit(),
				conf.getHighLimit(),
				escape_json(conf.getBinaryName()).c_str(),
				getDateNow().c_str(),
				nTotalCodeLines,
				nTotalExecutedLines
				) + json + "];";

		// Produce HTML outfile
		auto html = std::string((const char *)index_text_data, index_text_data_size);

		// .. And write both files to disk
		write_file((void *)json.c_str(), json.size(), (m_indexDirectory + "index.json").c_str());
		write_file((void *)html.c_str(), html.size(), (m_indexDirectory + "index.html").c_str());
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
	std::string imageFromPercent(double percent)
	{
		auto &conf = IConfiguration::getInstance();

		if (percent >= conf.getHighLimit())
			return "emerald.png";
		else if (percent > conf.getLowLimit())
			return "amber.png";
		else if (percent <= 1)
			return "snow.png";

		return "ruby.png";
	}

	std::string getDateNow()
	{
		time_t t;
		struct tm *tm;
		char date_buf[128];

		t = time(nullptr);
		tm = localtime(&t);
		strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", tm);

		return std::string(date_buf);
	}

	void writeHelperFiles(std::string dir)
	{
		write_file(icon_ruby_data, icon_ruby_data_size, "%s/ruby.png", dir.c_str());
		write_file(icon_amber_data, icon_amber_data_size, "%s/amber.png", dir.c_str());
		write_file(icon_emerald_data, icon_emerald_data_size, "%s/emerald.png", dir.c_str());
		write_file(icon_snow_data, icon_snow_data_size, "%s/snow.png", dir.c_str());
		write_file(icon_glass_data, icon_glass_data_size, "%s/glass.png", dir.c_str());
		write_file(css_text_data, css_text_data_size, "%s/bcov.css", dir.c_str());

		mkdir(fmt("%s/data", dir.c_str()).c_str(), 0755);
		mkdir(fmt("%s/data/js", dir.c_str()).c_str(), 0755);
		write_file(icon_ruby_data, icon_ruby_data_size, "%s/data/ruby.png", dir.c_str());
		write_file(icon_amber_data, icon_amber_data_size, "%s/data/amber.png", dir.c_str());
		write_file(icon_emerald_data, icon_emerald_data_size, "%s/data/emerald.png", dir.c_str());
		write_file(icon_snow_data, icon_snow_data_size, "%s/data/snow.png", dir.c_str());
		write_file(icon_glass_data, icon_glass_data_size, "%s/data/glass.png", dir.c_str());
		write_file(css_text_data, css_text_data_size, "%s/data/bcov.css", dir.c_str());
		write_file(tempo_text_data, tempo_text_data_size, "%s/data/js/tempo.min.js", dir.c_str());
		write_file(kcov_text_data, kcov_text_data_size, "%s/data/js/kcov.js", dir.c_str());
		write_file(jquery_text_data, jquery_text_data_size, "%s/data/js/jquery.min.js", dir.c_str());
		write_file(tablesorter_text_data, tablesorter_text_data_size, "%s/data/js/tablesorter.min.js", dir.c_str());
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
