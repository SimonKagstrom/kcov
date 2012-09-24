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
			FILENAME,
			FILE_LENGTH,
			UNCOVERED_LINES
		};

		enum OutputType
		{
			OUTPUT_COVERAGE,
			OUTPUT_PROFILER
		};

		virtual ~IConfiguration() {}

		virtual void printUsage() = 0;

		virtual std::string &getOutDirectory() = 0;

		virtual std::string &getBinaryName() = 0;

		virtual std::string &getBinaryPath() = 0;

		virtual enum SortType getSortType() = 0;

		virtual unsigned int getAttachPid() = 0;

		virtual unsigned int getLowLimit() = 0;

		virtual unsigned int getHighLimit() = 0;

		virtual unsigned int getPathStripLevel() = 0;

		virtual const char **getArgv() = 0;

		virtual std::map<unsigned int,std::string> &getExcludePattern() = 0;

		virtual std::map<unsigned int,std::string> &getOnlyIncludePattern() = 0;

		virtual std::map<unsigned int,std::string> &getOnlyIncludePath() = 0;

		virtual std::map<unsigned int,std::string> &getExcludePath() = 0;

		virtual enum OutputType getOutputType() = 0;

		virtual void setOutputType(enum OutputType) = 0;

		virtual bool parse(unsigned int argc, const char *argv[]) = 0;


		static IConfiguration &getInstance();
	};
}
