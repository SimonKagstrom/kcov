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
#include <unordered_map>
#include <iostream>
using namespace kcov;

class Configuration : public IConfiguration
{
public:
	Configuration()
	{
		m_outDirectory = "";
		m_binaryName = "";
		m_kernelCoveragePath = "/sys/kernel/debug/kprobe-coverage";
		m_programArgs = NULL;
		m_argc = 0;
		m_originalPathPrefix="";
		m_newPathPrefix="";
		m_printUncommon = false;

		setupDefaults();
	}


	const std::string &keyAsString(const std::string &key)
	{
		panic_if(m_strings.find(key) == m_strings.end(),
				"key %s not found", key.c_str());

		return m_strings[key];
	}

	int keyAsInt(const std::string &key)
	{
		panic_if(m_ints.find(key) == m_ints.end(),
				"key %s not found", key.c_str());

		return m_ints[key];
	}

	const std::vector<std::string> &keyAsList(const std::string &key)
	{
		panic_if(m_stringVectors.find(key) == m_stringVectors.end(),
				"key %s not found", key.c_str());

		return m_stringVectors[key];
	}

	bool usage(void)
	{
		printf("Usage: kcov [OPTIONS] out-dir in-file [args...]\n"
				"\n"
				"Where [OPTIONS] are\n"
				" -h, --help              this text\n"
				" -p, --pid=PID           trace PID instead of executing in-file,\n"
				"                         in-file is optional in this case\n"
				" -l, --limits=low,high   setup limits for low/high coverage (default %u,%u)\n"
				"\n"
				" --collect-only          Only collect coverage data (don't produce HTML/\n"
				"                         Cobertura output)\n"
				" --report-only           Produce output from stored databases, don't collect\n"
				"\n"
				" --include-path=path     comma-separated paths to include in the coverage report\n"
				" --exclude-path=path     comma-separated paths to exclude from the coverage\n"
				"                         report\n"
				" --include-pattern=pat   comma-separated path patterns to include in the\n"
				"                         coverage report\n"
				" --exclude-pattern=pat   comma-separated path patterns to exclude from the \n"
				"                         coverage report\n"
				"\n"
				" --coveralls-id=id       Travis CI job ID or secret repo_token for uploads to\n"
				"                         Coveralls.io\n"
				"%s"
				"\n"
				"Examples:\n"
				"  kcov /tmp/frodo ./frodo          # Check coverage for ./frodo\n"
				"  kcov --pid=1000 /tmp/frodo ./frodo     # Check coverage for PID 1000\n"
				"  kcov --include-pattern=/src/frodo/ /tmp/frodo ./frodo  # Only include files\n"
				"                                                         # including /src/frodo\n"
				"  kcov --collect-only /tmp/kcov ./frodo  # Collect coverage, don't report\n"
				"  kcov --report-only /tmp/kcov ./frodo   # Report coverage collected above\n"
				"",
				keyAsInt("low-limit"), keyAsInt("high-limit"),
				uncommonOptions().c_str());

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
				{"coveralls-id", required_argument, 0, 'T'},
				{"debug", required_argument, 0, 'D'},
				{"replace-src-path", required_argument, 0, 'R'},
				{"collect-only", no_argument, 0, 'C'},
				{"report-only", no_argument, 0, 'r'},
				{"python-parser", required_argument, 0, 'P'},
				{"bash-parser", required_argument, 0, 'B'},
				{"uncommon-options", no_argument, 0, 'U'},
				{"set-breakpoint", required_argument, 0, 'b'},
				/*{"write-file", required_argument, 0, 'w'}, Take back when the kernel stuff works */
				/*{"read-file", required_argument, 0, 'r'}, Ditto */
				{0,0,0,0}
		};
		unsigned int afterOpts = 0;
		unsigned int extraNeeded = 2;
		unsigned int lastArg;

		/* Scan through the parameters for an ELF file: That will be the
		 * second last argument in the list.
		 *
		 * After that it's arguments to the external program.
		 */
		for (lastArg = 1; lastArg < argc; lastArg++) {
			if (IParserManager::getInstance().matchParser(argv[lastArg]))
				break;
		}

		bool printUsage = false;

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
				printUsage = true;
				break;
			case 'U':
				m_printUncommon = true;
				break;
			case 'L':
				setKey("parse-solibs", 0);
				break;
			case 'F':
				setKey("daemonize-on-first-process-exit", 1);
				break;
			case 'p':
				if (!isInteger(std::string(optarg)))
					return usage();
				setKey("attach-pid", stoul(std::string(optarg)));
				extraNeeded = 1;
				break;
			case 't':
				m_title = optarg;
				break;
			case 'O':
				if (!isInteger(std::string(optarg)))
					return usage();

				setKey("output-interval", stoul(std::string(optarg)));
				break;
			case 'S':
			{
				if (!isInteger(std::string(optarg)))
					return usage();

				int pathStripLevel = stoul(std::string(optarg));
				if (pathStripLevel == 0)
					pathStripLevel = INT_MAX;
				setKey("path-strip-level", pathStripLevel);
				break;
			}
			case 'D':
				if (!isInteger(std::string(optarg)))
					return usage();
				g_kcov_debug_mask = stoul(std::string(optarg));
				break;
			case 'i':
				setKey("include-pattern", getCommaSeparatedList(std::string(optarg)));
				break;
			case 'P':
				setKey("python-command", optarg);
				break;
			case 'B':
				setKey("bash-command", optarg);
				break;
			case 'T':
				setKey("coveralls-id", optarg);
				break;
			case 'x':
				setKey("exclude-pattern", getCommaSeparatedList(std::string(optarg)));
				break;
			case 'I':
			{
				StrVecMap_t onlyIncludePath = getCommaSeparatedList(std::string(optarg));
				expandPath(onlyIncludePath);

				setKey("include-path", onlyIncludePath);
				break;
			}
			case 'X':
			{
				StrVecMap_t excludePath = getCommaSeparatedList(std::string(optarg));
				expandPath(excludePath);

				setKey("exclude-path", excludePath);
				break;
			}
			case 'C':
				setKey("running-mode", IConfiguration::MODE_COLLECT_ONLY);
				break;
			case 'r':
				setKey("running-mode", IConfiguration::MODE_REPORT_ONLY);
				break;
			case 'b': {
				StrVecMap_t vec = getCommaSeparatedList(std::string(optarg));

				for (StrVecMap_t::const_iterator it = vec.begin();
						it != vec.end();
						++it) {
					if (!string_is_integer(*it, 16))
						continue;

					m_fixedBreakpoints.push_back(string_to_integer(*it, 16));
				}

				break;
			}
			case 'l': {
				StrVecMap_t vec = getCommaSeparatedList(std::string(optarg));

				if (vec.size() != 2)
					return usage();

				if (!isInteger(vec[0]) || !isInteger(vec[1]))
					return usage();

				setKey("low-limit", stoul(vec[0]));
				setKey("high-limit", stoul(vec[1]));
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
			    
			    char* rp = ::realpath(m_newPathPrefix.c_str(), NULL);
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

		if (printUsage)
			return usage();

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
		m_argc = argc - afterOpts - 1;


		return true;
	}

	const std::string &getOutDirectory()
	{
		return m_outDirectory;
	}

	const std::string &getBinaryName()
	{
		return m_binaryName;
	}

	const std::string &getBinaryPath()
	{
		return m_binaryPath;
	}

	const std::string &getKernelCoveragePath()
	{
		return m_kernelCoveragePath;
	}

	std::list<uint64_t> getFixedBreakpoints()
	{
		return m_fixedBreakpoints;
	}

	const char **getArgv()
	{
		return m_programArgs;
	}

	unsigned int getArgc()
	{
		return m_argc;
	}

	const std::string& getOriginalPathPrefix()
	{
		return m_originalPathPrefix;
	}

	const std::string& getNewPathPrefix()
	{
		return m_newPathPrefix;
	}

	void setParseSolibs(bool on)
	{
		setKey("parse-solibs", on);
	}

	void registerListener(IListener &listener, const std::vector<std::string> &keys)
	{
		for (std::vector<std::string>::const_iterator it = keys.begin();
				it != keys.end();
				++it)
			m_listeners[*it] = &listener;
	}

	// "private", but we ignore that in the unit test
	typedef std::vector<std::string> StrVecMap_t;
	typedef std::pair<std::string, std::string> StringPair_t;

	typedef std::unordered_map<std::string, std::string> StringKeyMap_t;
	typedef std::unordered_map<std::string, int> IntKeyMap_t;
	typedef std::unordered_map<std::string, StrVecMap_t> StrVecKeyMap_t;

	typedef std::unordered_map<std::string, IListener *> ListenerMap_t;


	// Setup the default key:value pairs
	void setupDefaults()
	{
		setKey("python-command", "python");
		setKey("bash-command", "/bin/bash");
		setKey("path-strip-level", 2);
		setKey("attach-pid", 0);
		setKey("parse-solibs", 1);
		setKey("low-limit", 25);
		setKey("high-limit", 75);
		setKey("output-interval", 5000);
		setKey("daemonize-on-first-process-exit", 0);
		setKey("coveralls-id", "");
		setKey("running-mode", IConfiguration::MODE_COLLECT_AND_REPORT);
		setKey("exclude-pattern", StrVecMap_t());
		setKey("include-pattern", StrVecMap_t());
		setKey("exclude-path", StrVecMap_t());
		setKey("include-path", StrVecMap_t());
	}


	void setKey(const std::string &key, const std::string &val)
	{
		m_strings[key] = val;

		notifyKeyListeners(key);
	}

	void setKey(const std::string &key, int val)
	{
		m_ints[key] = val;

		notifyKeyListeners(key);
	}

	void setKey(const std::string &key, const std::vector<std::string> &val)
	{
		m_stringVectors[key] = val;

		notifyKeyListeners(key);
	}

	void notifyKeyListeners(const std::string &key)
	{
		ListenerMap_t::iterator it = m_listeners.find(key);

		if (it == m_listeners.end())
			return;

		// The callback should re-read the configuration item
		it->second->onConfigurationChanged(key);
	}

	std::string uncommonOptions()
	{
		if (!m_printUncommon)
			return " --uncommon-options      print uncommon options for --help\n";

		return fmt(
				" --replace-src-path=path replace the string found before the : with the string \n"
				"                         found after the :\n"
				" -t, --title=title       title for the coverage (default: filename)\n"
				" --path-strip-level=num  path levels to show for common paths (default: %d)\n"
				"\n"
				" --skip-solibs           don't parse shared libraries (default: parse solibs)\n"
				" --exit-first-process    exit when the first process exits, i.e., honor the\n"
				"                         behavior of daemons (default: wait until last)\n"
				" --output-interval=ms    Interval to produce output in milliseconds (0 to\n"
				"                         only output when kcov terminates, default %d)\n"
				"\n"
				" --debug=X               set kcov debugging level (max 31, default 0)\n"
				" --set-breakpoint=A[,..] manually set breakpoints\n"
				"\n"
				" --python-parser=cmd     Python parser to use (for python script coverage),\n"
				"                         default: %s\n"
				" --bash-parser=cmd       Bash parser to use (for bash/sh script coverage),\n"
				"                         default: %s\n",
				keyAsInt("path-strip-level"), keyAsInt("output-interval"), keyAsString("python-command").c_str(), keyAsString("bash-command").c_str()
				);
	}

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
		for (StrVecMap_t::iterator it = paths.begin();
				it != paths.end();
				++it) {
			std::string &s = *it;

			if (s[0] == '~')
				s = get_home() + s.substr(1, s.size());
			*it = s;
		}
	}

	StrVecMap_t getCommaSeparatedList(std::string str)
	{
		StrVecMap_t out;

		if (str.find(',') == std::string::npos) {
			out.push_back(str);
			return out;
		}

		size_t pos, lastPos;

		lastPos = 0;
		for (pos = str.find_first_of(",");
				pos != std::string::npos;
				pos = str.find_first_of(",", pos + 1)) {
			std::string cur = str.substr(lastPos, pos - lastPos);

			out.push_back(cur);
			lastPos = pos + 1;
		}
		out.push_back(str.substr(lastPos, str.size() - lastPos));

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

	StringKeyMap_t m_strings;
	IntKeyMap_t m_ints;
	StrVecKeyMap_t m_stringVectors;

	std::string m_outDirectory;
	std::string m_binaryName;
	std::string m_binaryPath;
	std::string m_kernelCoveragePath;
	const char **m_programArgs;
	unsigned int m_argc;
	std::string m_title;
	std::string m_originalPathPrefix;
	std::string m_newPathPrefix;
	bool m_printUncommon;
	std::list<uint64_t> m_fixedBreakpoints;

	ListenerMap_t m_listeners;
};


IConfiguration & IConfiguration::getInstance()
{
	static Configuration *g_instance;

	if (!g_instance)
		g_instance = new Configuration();

	return *g_instance;
}
