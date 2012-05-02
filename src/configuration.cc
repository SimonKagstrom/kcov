#include <configuration.hh>
#include <utils.hh>

#include <getopt.h>
#include <limits.h>
#include <map>
#include <list>
#include <string>
#include <stdexcept>

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
	}

	bool usage(void)
	{
		printf("Usage: kcov [OPTIONS] out-dir in-file [args...]\n"
				"\n"
				"Where [OPTIONS] are\n"
				" -h, --help             this text\n"
				" -p, --pid=PID          trace PID instead of executing in-file,\n"
				"                        in-file is optional in this case\n"
				" -s, --sort-type=type   how to sort files: f[ilename] (default), p[ercent]\n"
				" -l, --limits=low,high  setup limits for low/high coverage (default %u,%u)\n"
				" -t, --m_title=title      m_title for the coverage (default: filename)\n"
				" --path-strip-level=num path levels to show for common paths (default: %u)\n"
				" --include-path=path    comma-separated paths to include in the report\n"
				" --exclude-path=path    comma-separated paths to exclude from the report\n"
				" --include-pattern=pat  comma-separated path patterns to include in the report\n"
				" --exclude-pattern=pat  comma-separated path patterns to exclude from the report\n"
				"\n"
				"Examples:\n"
				"  kcov /tmp/frodo ./frodo          # Check coverage for ./frodo\n"
				"  kcov --pid=1000 /tmp/frodo       # Check coverage for PID 1000\n"
				"  kcov --include-pattern=/src/frodo/ /tmp/frodo ./frodo  # Only include files\n"
				"                                                         # including /src/frodo\n"
				"",
				m_lowLimit, m_highLimit, m_pathStripLevel);
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
				{"m_title", required_argument, 0, 't'},
				{"path-strip-level", required_argument, 0, 'S'},
				{"exclude-pattern", required_argument, 0, 'x'},
				{"include-pattern", required_argument, 0, 'i'},
				{"exclude-path", required_argument, 0, 'X'},
				{"include-path", required_argument, 0, 'I'},
				{"debug", required_argument, 0, 'D'},
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
			if (file_is_elf(argv[lastArg]))
				break;
		}

		/* Hooray for reentrancy... */
		optind = 0;
		optarg = 0;
		while (1) {
			char *endp;
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
			case 'p':
				m_ptracePid = strtoul(optarg, &endp, 0);
				extraNeeded = 1;
				break;
			case 's':
				if (*optarg == 'p' || *optarg == 'P')
					m_sortType = PERCENTAGE;
				else if (*optarg != 'f' && *optarg != 'F')
					return usage();
				break;
			case 't':
				m_title = optarg;
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
				break;
			case 'X':
				m_excludePath = getCommaSeparatedList(std::string(optarg));
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
			default:
				error("Unrecognized option: -%c\n", optopt);
				break;
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
	StrVecMap_t m_excludePattern;
	StrVecMap_t m_onlyIncludePattern;
	StrVecMap_t m_excludePath;
	StrVecMap_t m_onlyIncludePath;
};


IConfiguration & IConfiguration::getInstance()
{
	static Configuration *g_instance;

	if (!g_instance)
		g_instance = new Configuration();

	return *g_instance;
}
