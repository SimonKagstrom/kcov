#include <configuration.hh>
#include <file-parser.hh>
#include <utils.hh>
#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
#include <map>
#include <list>
#include <string>
#include <stdexcept>
#include <iostream>
using namespace kcov;

class Configuration : public IConfiguration
{
public:
	Configuration()
	{
		m_outDirectory = "";
		m_binaryName = "";
		m_lowLimit = 25;
		m_highLimit = 75;
		m_pathStripLevel = 2;
		m_ptracePid = 0;
		m_sortType = FILENAME;
		m_outputType = OUTPUT_COVERAGE;
		m_originalPathPrefix="";
		m_newPathPrefix="";
		m_parseSolibs = true;
		m_exitFirstProcess = false;
		m_outputInterval = 1000;
		m_runMode = IConfiguration::MODE_COLLECT_AND_REPORT;
	}

	bool usage(void)
	{
		printf("Usage: kcov [OPTIONS] out-dir in-file [args...]\n"
				"\n"
				"Where [OPTIONS] are\n"
				" -h, --help              this text\n"
				" -p, --pid=PID           trace PID instead of executing in-file,\n"
				"                         in-file is optional in this case\n"
				" -s, --sort-type=type    how to sort files: f[ilename] (default), p[ercent],\n"
				"                         r[everse-percent], u[ncovered-lines], l[ines]\n"
				" -l, --limits=low,high   setup limits for low/high coverage (default %u,%u)\n"
				" -t, --title=title       title for the coverage (default: filename)\n"
				" --output-interval=ms    Interval to produce output in milliseconds (0 to\n"
				"                         only output when kcov terminates, default %u)\n"
				" --path-strip-level=num  path levels to show for common paths (default: %u)\n"
				" --skip-solibs           don't parse shared libraries (default: parse solibs)\n"
				" --exit-first-process    exit when the first process exits, i.e., honor the\n"
				"                         behavior of daemons (default: wait until last)\n"
				" --collect-only          Only collect coverage data (don't produce HTML/\n"
				"                         Cobertura output)\n"
				" --report-only           Produce output from stored databases, don't collect\n"
				" --include-path=path     comma-separated paths to include in the report\n"
				" --exclude-path=path     comma-separated paths to exclude from the report\n"
				" --include-pattern=pat   comma-separated path patterns to include in the report\n"
				" --exclude-pattern=pat   comma-separated path patterns to exclude from the \n"
				"                         report\n"
				" --replace-src-path=path replace the string found before the : with the string \n"
				"                         found after the :\n"
				"\n"
				"Examples:\n"
				"  kcov /tmp/frodo ./frodo          # Check coverage for ./frodo\n"
				"  kcov --pid=1000 /tmp/frodo       # Check coverage for PID 1000\n"
				"  kcov --include-pattern=/src/frodo/ /tmp/frodo ./frodo  # Only include files\n"
				"                                                         # including /src/frodo\n"
				"  kcov --collect-only /tmp/kcov ./frodo  # Collect coverage, don't report\n"
				"  kcov --report-only /tmp/kcov ./frodo   # Report coverage collected above\n"
				"",
				m_lowLimit, m_highLimit, m_outputInterval, m_pathStripLevel);
		return false;
	}

	void printUsage()
	{
		usage();
	}

	bool parse(unsigned int argc, const char *argv[])
	{
		static const struct option long_options[] = {
				{"help", no_argument, 0, 'h'},
				{"pid", required_argument, 0, 'p'},
				{"sort-type", required_argument, 0, 's'},
				{"limits", required_argument, 0, 'l'},
				{"output-interval", required_argument, 0, 'O'},
				{"title", required_argument, 0, 't'},
				{"path-strip-level", required_argument, 0, 'S'},
				{"skip-solibs", no_argument, 0, 'L'},
				{"exit-first-process", no_argument, 0, 'F'},
				{"exclude-pattern", required_argument, 0, 'x'},
				{"include-pattern", required_argument, 0, 'i'},
				{"exclude-path", required_argument, 0, 'X'},
				{"include-path", required_argument, 0, 'I'},
				{"debug", required_argument, 0, 'D'},
				{"replace-src-path", required_argument, 0, 'R'},
				{"collect-only", no_argument, 0, 'C'},
				{"report-only", no_argument, 0, 'r'},
				/*{"write-file", required_argument, 0, 'w'}, Take back when the kernel stuff works */
				/*{"read-file", required_argument, 0, 'r'}, Ditto */
				{0,0,0,0}
		};
		unsigned int afterOpts = 0;
		unsigned int extraNeeded = 2;
		unsigned int lastArg;


		m_onlyIncludePath.clear();
		m_onlyIncludePattern.clear();
		m_excludePath.clear();
		m_excludePattern.clear();

		/* Scan through the parameters for an ELF file: That will be the
		 * second last argument in the list.
		 *
		 * After that it's arguments to the external program.
		 */
		for (lastArg = 1; lastArg < argc; lastArg++) {
			if (IParserManager::getInstance().matchParser(argv[lastArg]))
				break;
		}

		/* Hooray for reentrancy... */
		optind = 0;
		optarg = 0;
		while (1) {
			int option_index = 0;
			int c;

			c = getopt_long (lastArg, (char **)argv,
					"hp:s:l:t:", long_options, &option_index);

			/* No more options */
			if (c == -1)
				break;

			switch (c) {
			case 0:
				break;
			case 'h':
				return usage();
			case 'L':
				m_parseSolibs = false;
				break;
			case 'F':
				m_exitFirstProcess = true;
				break;
			case 'p':
				if (!isInteger(std::string(optarg)))
					return usage();
				m_ptracePid = stoul(std::string(optarg));
				extraNeeded = 1;
				break;
			case 's':
				if (*optarg == 'p' || *optarg == 'P')
					m_sortType = PERCENTAGE;
				else if (*optarg == 'r' || *optarg == 'R')
					m_sortType = REVERSE_PERCENTAGE;
				else if (*optarg == 'u' || *optarg == 'U')
					m_sortType = UNCOVERED_LINES;
				else if (*optarg == 'l' || *optarg == 'L')
					m_sortType = FILE_LENGTH;
				else if (*optarg != 'f' && *optarg != 'F')
					return usage();
				break;
			case 't':
				m_title = optarg;
				break;
			case 'O':
				if (!isInteger(std::string(optarg)))
					return usage();

				m_outputInterval = stoul(std::string(optarg));
				break;
			case 'S':
				if (!isInteger(std::string(optarg)))
					return usage();
				m_pathStripLevel = stoul(std::string(optarg));
				if (m_pathStripLevel == 0)
					m_pathStripLevel = ~0;
				break;
			case 'D':
				if (!isInteger(std::string(optarg)))
					return usage();
				g_kcov_debug_mask = stoul(std::string(optarg));
				break;
			case 'i':
				m_onlyIncludePattern = getCommaSeparatedList(std::string(optarg));
				break;
			case 'x':
				m_excludePattern = getCommaSeparatedList(std::string(optarg));
				break;
			case 'I':
				m_onlyIncludePath = getCommaSeparatedList(std::string(optarg));
				expandPath(m_onlyIncludePath);
				break;
			case 'X':
				m_excludePath = getCommaSeparatedList(std::string(optarg));
				expandPath(m_excludePath);
				break;
			case 'C':
				m_runMode = IConfiguration::MODE_COLLECT_ONLY;
				break;
			case 'r':
				m_runMode = IConfiguration::MODE_REPORT_ONLY;
				break;
			case 'l': {
				StrVecMap_t vec = getCommaSeparatedList(std::string(optarg));

				if (vec.size() != 2)
					return usage();

				if (!isInteger(vec[0]) || !isInteger(vec[1]))
					return usage();

				m_lowLimit = stoul(vec[0]);
				m_highLimit = stoul(vec[1]);
				break;
			}
			case 'R': {
			  std::string tmpArg = std::string(optarg);
			  size_t tokenPosFront = tmpArg.find_first_of(":");
			  size_t tokenPosBack  = tmpArg.find_last_of(":");

			  if ((tokenPosFront != std::string::npos) && 
			      (tokenPosFront == tokenPosBack)) {

			    m_originalPathPrefix = tmpArg.substr(0, tokenPosFront);
			    m_newPathPrefix = tmpArg.substr(tokenPosFront + 1);
			    
			    char* rp = ::realpath(m_newPathPrefix.c_str(), nullptr);
			    if (rp) {
			      free((void*) rp);
			    }
			    else {
			      error("%s is not a valid path.\n", m_newPathPrefix.c_str());
			    }
			    
			  }
			  else {
			    error("%s is formatted incorrectly\n", tmpArg.c_str());
			    
			  }
			  break;
			}
			default:
				error("Unrecognized option: -%c\n", optopt);
				return usage();
			}
		}

		afterOpts = optind;

		/* When tracing by PID, the filename is optional */
		if (argc < afterOpts + extraNeeded)
			return usage();

		m_outDirectory = argv[afterOpts];
		if (m_outDirectory[m_outDirectory.size() - 1] != '/')
			m_outDirectory += "/";
		if (argc >= afterOpts + 2)
		{
			StringPair_t tmp = splitPath(argv[afterOpts + 1]);

			m_binaryPath = tmp.first;
			m_binaryName = tmp.second;
		}
		m_programArgs = &argv[afterOpts + 1];


		return true;
	}

	std::string &getOutDirectory()
	{
		return m_outDirectory;
	}

	std::string &getBinaryName()
	{
		return m_binaryName;
	}

	std::string &getBinaryPath()
	{
		return m_binaryPath;
	}

	unsigned int getAttachPid()
	{
		return m_ptracePid;
	}

	unsigned int getLowLimit()
	{
		return m_lowLimit;
	}

	unsigned int getHighLimit()
	{
		return m_highLimit;
	}

	const char **getArgv()
	{
		return m_programArgs;
	}

	enum SortType getSortType()
	{
		return m_sortType;
	}

	std::map<unsigned int, std::string> &getExcludePattern()
	{
		return m_excludePattern;
	}

	std::map<unsigned int, std::string> &getOnlyIncludePattern()
	{
		return m_onlyIncludePattern;
	}

	std::map<unsigned int, std::string> &getOnlyIncludePath()
	{
		return m_onlyIncludePath;
	}

	std::map<unsigned int, std::string> &getExcludePath()
	{
		return m_excludePath;
	}

	unsigned int getPathStripLevel()
	{
		return m_pathStripLevel;
	}

	enum OutputType getOutputType()
	{
		return m_outputType;
	}

	const std::string& getOriginalPathPrefix()
	{
		return m_originalPathPrefix;
	}

	const std::string& getNewPathPrefix()
	{
		return m_newPathPrefix;
	}

	bool getParseSolibs()
	{
		return m_parseSolibs;
	}

	void setParseSolibs(bool on)
	{
		m_parseSolibs = on;
	}

	bool getExitFirstProcess()
	{
		return m_exitFirstProcess;
	}

	void setOutputType(enum OutputType type)
	{
		m_outputType = type;
	}

	unsigned int getOutputInterval()
	{
		return m_outputInterval;
	}

	RunMode_t getRunningMode()
	{
		return m_runMode;
	}

	// "private", but we ignore that in the unit test
	typedef std::map<unsigned int, std::string> StrVecMap_t;
	typedef std::pair<std::string, std::string> StringPair_t;

	bool isInteger(std::string str)
	{
		size_t pos;

		try
		{
			stoul(str, &pos);
		}
		catch(std::invalid_argument &e)
		{
			return false;
		}

		return pos == str.size();
	}

	void expandPath(StrVecMap_t &paths)
	{
		for (auto &it : paths) {
			std::string &s = it.second;

			if (s[0] == '~')
				s = get_home() + s.substr(1, s.size());
			it.second = s;
		}
	}

	StrVecMap_t getCommaSeparatedList(std::string str)
	{
		StrVecMap_t out;

		if (str.find(',') == std::string::npos) {
			out[0] = str;
			return out;
		}

		size_t pos, lastPos;
		unsigned int n = 0;

		lastPos = 0;
		for (pos = str.find_first_of(",");
				pos != std::string::npos;
				pos = str.find_first_of(",", pos + 1)) {
			std::string cur = str.substr(lastPos, pos - lastPos);

			out[n++] = cur;
			lastPos = pos;
		}
		out[n] = str.substr(lastPos + 1, str.size() - lastPos);

		return out;
	}

	StringPair_t splitPath(const char *pathStr)
	{
		StringPair_t out;
		std::string path(pathStr);

		size_t pos = path.rfind('/');

		out.first = "";
		out.second = path;
		if (pos != std::string::npos) {
			out.first= path.substr(0, pos + 1);
			out.second = path.substr(pos + 1, std::string::npos);
		}

		return out;
	}


	unsigned int m_lowLimit;
	unsigned int m_highLimit;
	unsigned int m_pathStripLevel;
	enum SortType m_sortType;
	unsigned int m_ptracePid;
	std::string m_outDirectory;
	std::string m_binaryName;
	std::string m_binaryPath;
	const char **m_programArgs;
	std::string m_title;
	std::string m_originalPathPrefix;
	std::string m_newPathPrefix;
	bool m_parseSolibs;
	bool m_exitFirstProcess;
	StrVecMap_t m_excludePattern;
	StrVecMap_t m_onlyIncludePattern;
	StrVecMap_t m_excludePath;
	StrVecMap_t m_onlyIncludePath;
	enum OutputType m_outputType;
	unsigned int m_outputInterval;
	RunMode_t m_runMode;
};


IConfiguration & IConfiguration::getInstance()
{
	static Configuration *g_instance;

	if (!g_instance)
		g_instance = new Configuration();

	return *g_instance;
}
