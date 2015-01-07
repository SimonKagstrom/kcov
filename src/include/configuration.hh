#pragma once

#include <string>
#include <list>
#include <vector>

namespace kcov
{
	class IConfiguration
	{
	public:
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

		virtual const std::string &getOutDirectory() = 0;

		virtual const std::string &getBinaryName() = 0;

		virtual const std::string &getBinaryPath() = 0;

		/**
		 * Return the secret repo token for coveralls.io,
		 * or the Travis CI service ID.
		 *
		 * @return the token, or "" if it isn't set.
		 */
		virtual const std::string &getCoverallsId() = 0;

		virtual const std::string &getKernelCoveragePath() = 0;

		virtual const std::string &getPythonCommand() const = 0;

		virtual const std::string &getBashCommand() const = 0;

		virtual std::list<uint64_t> getFixedBreakpoints() = 0;

		virtual unsigned int getAttachPid() = 0;

		virtual unsigned int getLowLimit() = 0;

		virtual unsigned int getHighLimit() = 0;

		virtual unsigned int getPathStripLevel() = 0;

		virtual const char **getArgv() = 0;

		virtual unsigned int getArgc() = 0;

		virtual const std::vector<std::string> &getExcludePattern() = 0;

		virtual const std::vector<std::string> &getOnlyIncludePattern() = 0;

		virtual const std::vector<std::string> &getOnlyIncludePath() = 0;

		virtual const std::vector<std::string> &getExcludePath() = 0;

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
