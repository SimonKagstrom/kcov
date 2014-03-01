#pragma once

#include <string>
#include <map>

namespace kcov
{
	class IConfiguration
	{
	public:
		enum SortType
		{
			PERCENTAGE,
			REVERSE_PERCENTAGE,
			FILENAME,
			FILE_LENGTH,
			UNCOVERED_LINES
		};

		enum OutputType
		{
			OUTPUT_COVERAGE,
			OUTPUT_PROFILER
		};

		typedef enum
		{
			MODE_COLLECT_ONLY       = 1,
			MODE_REPORT_ONLY        = 2,
			MODE_COLLECT_AND_REPORT = 3,
		} RunMode_t;

		virtual ~IConfiguration() {}

		virtual void printUsage() = 0;

		virtual std::string &getOutDirectory() = 0;

		virtual std::string &getBinaryName() = 0;

		virtual std::string &getBinaryPath() = 0;

		virtual const std::string &getPythonCommand() const = 0;

		virtual enum SortType getSortType() = 0;

		virtual unsigned int getAttachPid() = 0;

		virtual unsigned int getLowLimit() = 0;

		virtual unsigned int getHighLimit() = 0;

		virtual unsigned int getPathStripLevel() = 0;

		virtual const char **getArgv() = 0;

		virtual unsigned int getArgc() = 0;

		virtual std::map<unsigned int,std::string> &getExcludePattern() = 0;

		virtual std::map<unsigned int,std::string> &getOnlyIncludePattern() = 0;

		virtual std::map<unsigned int,std::string> &getOnlyIncludePath() = 0;

		virtual std::map<unsigned int,std::string> &getExcludePath() = 0;

		virtual enum OutputType getOutputType() = 0;
		virtual const std::string& getNewPathPrefix() = 0;
		virtual const std::string& getOriginalPathPrefix() = 0;
		virtual void setOutputType(enum OutputType) = 0;

		virtual bool getParseSolibs() = 0;

		virtual void setParseSolibs(bool on) = 0;

		virtual bool getExitFirstProcess() = 0;

		virtual unsigned int getOutputInterval() = 0;

		virtual RunMode_t getRunningMode() = 0;


		virtual bool parse(unsigned int argc, const char *argv[]) = 0;


		static IConfiguration &getInstance();
	};
}
