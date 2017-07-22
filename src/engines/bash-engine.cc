#include <file-parser.hh>
#include <engine.hh>
#include <configuration.hh>
#include <output-handler.hh>
#include <lineid.hh>
#include <utils.hh>
#include <generated-data-base.hh>
#include <source-file-cache.hh>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#include <list>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include <swap-endian.hh>

#include "script-engine-base.hh"

using namespace kcov;

extern GeneratedData bash_helper_data;
extern GeneratedData bash_helper_debug_trap_data;
extern GeneratedData bash_redirector_library_data;

enum InputType
{
	INPUT_NORMAL,       // No multiline stuff
	INPUT_QUOTE,        // "
	INPUT_SINGLE_QUOTE, // '
	INPUT_MULTILINE,    // backslash backslash
};

class BashEngine : public ScriptEngineBase
{
public:
	BashEngine() :
		ScriptEngineBase(),
		m_child(0),
		m_stderr(NULL),
		m_stdout(NULL),
		m_bashSupportsXtraceFd(false),
		m_inputType(INPUT_NORMAL)
	{
	}

	~BashEngine()
	{
		kill(SIGTERM);
	}

	bool start(IEventListener &listener, const std::string &executable)
	{
		int stderrPipe[2];
		int stdoutPipe[2];

		if (pipe(stderrPipe) < 0) {
			error("Failed to open pipe");

			return false;
		}

		if (pipe(stdoutPipe) < 0) {
			error("Failed to open pipe");
			close(stderrPipe[0]);
			close(stderrPipe[1]);

			return false;
		}

		m_bashSupportsXtraceFd = bashCanHandleXtraceFd();

		std::string helperPath =
				IOutputHandler::getInstance().getBaseDirectory() + "bash-helper.sh";
		std::string helperDebugTrapPath =
				IOutputHandler::getInstance().getBaseDirectory() + "bash-helper-debug-trap.sh";
		std::string redirectorPath =
				IOutputHandler::getInstance().getBaseDirectory() + "libbash_execve_redirector.so";

		if (write_file(bash_helper_data.data(), bash_helper_data.size(),
				"%s", helperPath.c_str()) < 0) {
				error("Can't write helper");

				return false;
		}
		if (write_file(bash_helper_debug_trap_data.data(), bash_helper_debug_trap_data.size(),
				"%s", helperDebugTrapPath.c_str()) < 0) {
				error("Can't write helper");

				return false;
		}
		if (write_file(bash_redirector_library_data.data(), bash_redirector_library_data.size(),
				"%s", redirectorPath.c_str()) < 0) {
				error("Can't write redirector library at %s", redirectorPath.c_str());

				return false;
		}

		m_listener = &listener;

		/* Launch child */
		m_child = fork();
		if (m_child > 0) {

			// Close the parents write end of the pipe
			close(stderrPipe[1]);
			close(stdoutPipe[1]);

			// And open a FILE * to stderr
			m_stderr = fdopen(stderrPipe[0], "r");
			if (!m_stderr) {
				error("Can't reopen the stderr pipe");
				return false;
			}
			m_stdout = fdopen(stdoutPipe[0], "r");
			if (!m_stderr) {
				error("Can't reopen the stdout pipe");
				fclose(m_stderr);

				return false;
			}

		} else if (m_child == 0) {
			IConfiguration &conf = IConfiguration::getInstance();
			const char **argv = conf.getArgv();
			unsigned int argc = conf.getArgc();
			int xtraceFd = 782; // Typical bash users use 3,4 etc but not high fd numbers (?)

			const std::string command = conf.keyAsString("bash-command");
			bool usePS4 = conf.keyAsInt("bash-use-ps4");

			// Revert to stderr if this bash version can't handle BASH_XTRACE
			if (usePS4 && !m_bashSupportsXtraceFd)
				xtraceFd = 2;

			if (dup2(stderrPipe[1], xtraceFd) < 0) {
			    perror("Failed to exchange stderr for pipe");
			    return false;
			}
			if (dup2(stdoutPipe[1], 1) < 0) {
			    perror("Failed to exchange stderr for pipe");
			    return false;
			}

			/* Close the childs old write end of the pipe */
			close(stderrPipe[1]);
			close(stdoutPipe[1]);

			/* Close the childs read end of the pipe */
			close(stderrPipe[0]);
			close(stdoutPipe[0]);

			doSetenv(fmt("KCOV_BASH_XTRACEFD=%d", xtraceFd));
			// Export the bash command for use in the bash-execve-redirector library
			doSetenv(fmt("KCOV_BASH_COMMAND=%s", command.c_str()));

			/* Set up PS4 for tracing */
			if (usePS4) {
				doSetenv(fmt("BASH_ENV=%s", helperPath.c_str()));
				doSetenv(fmt("BASH_XTRACEFD=%d", xtraceFd));
				doSetenv("PS4=kcov@${BASH_SOURCE}@${LINENO}@");
			} else {
				// Use DEBUG trap
				doSetenv(fmt("BASH_ENV=%s", helperDebugTrapPath.c_str()));
				doSetenv(fmt("KCOV_BASH_USE_DEBUG_TRAP=1"));
			}


			// And preload it!
			if (conf.keyAsInt("bash-handle-sh-invocation"))
				doSetenv(std::string("LD_PRELOAD=" + redirectorPath));

			// Make a copy of the vector, now with "bash -x" first
			char **vec;
			int argcStart = usePS4 ? 2 : 1;
			vec = (char **)xmalloc(sizeof(char *) * (argc + 3));
			vec[0] = xstrdup(conf.keyAsString("bash-command").c_str());

			if (usePS4)
				vec[1] = xstrdup("-x");
			for (unsigned i = 0; i < argc; i++)
				vec[argcStart + i] = xstrdup(argv[i]);

			/* Execute the script */
			if (execv(vec[0], vec)) {
				perror("Failed to execute script");

				free(vec);

				return false;
			}
		} else if (m_child < 0) {
			perror("fork");

			return false;
		}

		parseFile(executable);

		return true;
	}

	bool checkEvents()
	{
		char *curLine = NULL;
		ssize_t len;
		size_t linecap = 0;

		// First printout any collected stdout data
		handleStdout();

		len = getline(&curLine, &linecap, m_stderr);
		if (len < 0)
			return false;

		std::string cur(curLine);
		// Line markers always start with kcov@

		size_t kcovStr = cur.find("kcov@");
		enum InputType ip = getInputType(cur);
		if (kcovStr == std::string::npos) {
			if (m_inputType == INPUT_NORMAL)
				fprintf(stderr, "%s", cur.c_str());
			if (ip == INPUT_SINGLE_QUOTE && m_inputType == INPUT_SINGLE_QUOTE)
				m_inputType = INPUT_NORMAL;

			return true;
		}

		m_inputType = ip;

		std::vector<std::string> parts = split_string(cur.substr(kcovStr), "@");

		// kcov@FILENAME@LINENO@...
		if (parts.size() < 3)
			return true;

		// Resolve filename (might be relative)
		const std::string &filename = get_real_path(parts[1]);
		const std::string &lineNo = parts[2];

		// Skip the helper libraries
		if (filename.find("bash-helper.sh") != std::string::npos)
			return true;
		if (filename.find("bash-helper-debug-trap.sh") != std::string::npos)
			return true;

		if (!string_is_integer(lineNo)) {
			error("%s is not an integer", lineNo.c_str());

			return false;
		}

		if (!m_reportedFiles[filename]) {
			m_reportedFiles[filename] = true;

			for (FileListenerList_t::const_iterator it = m_fileListeners.begin();
					it != m_fileListeners.end();
					++it)
				(*it)->onFile(File(filename, IFileParser::FLG_NONE));

			parseFile(filename);
		}

		if (m_listener) {
			uint64_t address = 0;
			Event ev;

			LineIdToAddressMap_t::iterator it = m_lineIdToAddress.find(getLineId(filename, string_to_integer(lineNo)));
			if (it != m_lineIdToAddress.end())
				address = it->second;

			ev.type = ev_breakpoint;
			ev.addr = address;
			ev.data = 1;

			m_listener->onEvent(ev);
		}

		return true;
	}

	bool continueExecution()
	{
		if (checkEvents())
			return true;

		// Otherwise wait for child
		int status;
		int rv;

		rv = waitpid(m_child, &status, WNOHANG);
		if (rv != m_child) {
			return true;
		}
		else {
			// Child exited, let's make sure that we have all the events/stdout.
			checkEvents();
		}


		if (WIFEXITED(status)) {
			reportEvent(ev_exit_first_process, WEXITSTATUS(status));
		} else {
			warning("Other status: 0x%x\n", status);
			reportEvent(ev_error, -1);
		}

		return false;
	}

	void kill(int signal)
	{
		if (m_child == 0)
			return;

		::kill(m_child, signal);
	}

	std::string getParserType()
	{
		return "bash";
	}

	unsigned int matchParser(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		std::string s((const char *)data, 80);

		if (filename.size() >= 3 && filename.substr(filename.size() - 3, filename.size()) == ".sh")
			return 200;

		if (s.find("#!/bin/bash") != std::string::npos)
			return 400;
		if (s.find("#!/bin/sh") != std::string::npos)
			return 300;
		if (s.find("bash") != std::string::npos)
			return 100;
		if (s.find("sh") != std::string::npos)
			return 50;

		return match_none;
	}


private:
	// Printout lines to stdout, except kcov markers
	void handleStdout()
	{
		char *curLine = NULL;
		size_t linecap = 0;

		while (file_readable(m_stdout, 0) && getline(&curLine, &linecap, m_stdout) > 0)
		{
			/* Check for line markers to filter these away.
			 *
			 * For some reason, redirection sometimes give kkcov@..., so filter that
			 * in addition to the obvious stuff
			 */
			size_t kcovMarker = m_bashSupportsXtraceFd ? -1 : std::string(curLine).find("kcov@");

			if (kcovMarker != 0 && kcovMarker != 1)
			{
				printf("%s", curLine);
			}
		}
		free(curLine);
	}

	bool bashCanHandleXtraceFd()
	{
		FILE *fp;
		bool out = false;
		IConfiguration &conf = IConfiguration::getInstance();
		std::string cmd = conf.keyAsString("bash-command") + " --version";

		/* Has input via stderr been forced regardless of bash version?
		 * Basically for testing only
		 */
		if (conf.keyAsInt("bash-force-stderr-input"))
			return false;

		fp = popen(cmd.c_str(), "r");
		if (!fp)
			return false;

		// Read the first line
		char *line = NULL;
		ssize_t len;
		size_t linecap = 0;

		len = getline(&line, &linecap, fp);
		// Let's play safe
		if (len > 0) {
			std::string cur(line);
			size_t where = cur.find("version ");
			if (where != std::string::npos) {
				std::string versionStr = cur.substr(where + strlen("version "));

				// Bash 4.2 and above supports BASH_XTRACEFD
				if (versionStr.size() > 4 && versionStr[0] >= '4' && versionStr[2] >= '2')
					out = true;
			}
		}

		pclose(fp);

		return out;
	}


	enum InputType getInputType(const std::string &str)
	{
		enum InputType out = INPUT_NORMAL;

		size_t singleQuotes = std::count(str.begin(), str.end(), '\'');

		if (singleQuotes == 1)
			out = INPUT_SINGLE_QUOTE;

		return out;
	}

	void doSetenv(const std::string &val)
	{
		// "Leak" some bytes, but the environment needs that
		char *envString = (char *)xmalloc(val.size() + 1);

		strcpy(envString, val.c_str());

		putenv(envString);
	}

	// Sweep through lines in a file to determine what is valid code
	void parseFile(const std::string &filename)
	{
		if (!m_listener)
			return;
		ISourceFileCache &cache = ISourceFileCache::getInstance();

		std::vector<std::string> lines = cache.getLines(filename);

		// Compute hash for this file
		uint32_t crc = cache.getCrc(filename);

		IConfiguration &conf = IConfiguration::getInstance();

		if (conf.keyAsInt("bash-use-basic-parser"))
			parseFileBasic(filename, lines, crc);
		else
			parseFileFull(filename, lines, crc);
	}

	void parseFileBasic(const std::string &filename, const std::vector<std::string> &lines, uint32_t crc)
	{
		unsigned int lineNo = 0;

		for (std::vector<std::string>::const_iterator it = lines.begin();
				it != lines.end();
				++it) {
			std::string s = trim_string(*it);

			lineNo++;

			kcov_debug(ENGINE_MSG, "bash_line %2d: %s\n", lineNo, s.c_str());

			// Remove comments
			size_t comment = s.find("#");
			if (comment != std::string::npos) {

				// But not $# or ${#variable}...
				if ((comment >= 1 && s[comment - 1] == '$') ||
						(comment >= 2 && s.rfind("${", comment) != std::string::npos && s.find("}", comment) != std::string::npos)) {
					// Do nothing
				// Nor if in a string
				} else if (comment >= 1
				&& (s.find("\"") < comment || s.find("'") < comment)) {
					// Do nothing
				} else {
					s = trim_string(s);
					s = s.substr(0, comment);
				}
			}

			// Empty line, ignore
			if (s == "")
				continue;

			// While, if, switch endings
			if (s == "esac" ||
					s == "fi" ||
					s == "do" ||
					s == "done" ||
					s == "else" ||
					s == "then" ||
					s == "}" ||
					s == "{")
				continue;

			// Functions
			if (s.find("function") == 0)
				continue;

			fileLineFound(crc, filename, lineNo);
		}
	}

	void parseFileFull(const std::string &filename, const std::vector<std::string> &lines, uint32_t crc)
	{
		unsigned int lineNo = 0;
		enum { none, backslash, quote, heredoc } state = none;
		bool caseActive = false;
		bool arithmeticActive = false;
		std::string heredocMarker;

		for (std::vector<std::string>::const_iterator it = lines.begin();
				it != lines.end();
				++it) {
			std::string s = trim_string(*it);

			lineNo++;

			kcov_debug(ENGINE_MSG, "bash_line %2d: %s\n", lineNo, s.c_str());

			// Remove comments
			size_t comment = s.find("#");
			if (comment != std::string::npos) {

				// But not $# or ${#variable}...
				if ((comment >= 1 && s[comment - 1] == '$') ||
						(comment >= 2 && s.rfind("${", comment) != std::string::npos && s.find("}", comment) != std::string::npos)) {
					// Do nothing
				// Nor if in a string
				} else if (comment >= 1
				&& (s.find("\"") < comment || s.find("'") < comment)) {
					// Do nothing
				} else {
					s = trim_string(s);
					s = s.substr(0, comment);
				}
			}

			// Empty line, ignore
			if (s == "")
				continue;

			if (s.size() >= 2 &&
					s[0] == ';' && s[1] == ';')
				continue;

			// Empty braces
			if ((s[0] == '{' || s[0] == '}') && s.size() == 1)
				continue;

			// While, if, switch endings
			if (s == "esac" ||
					s == "fi" ||
					s == "do" ||
					s == "done" ||
					s == "else" ||
					s == "then")
				continue;

			// Functions
			if (s.find("function") == 0)
				continue;

			// fn() { .... Yes, regexes would have been better
			size_t fnPos = s.find("()");
			if (fnPos != std::string::npos) {
				std::string substr = s.substr(0, fnPos);

				// Should be a single word (the function name)
				if (substr.find(" ") == std::string::npos)
					continue;
			}


			// Handle backslashes - only the first line is code
			if (state == backslash) {
				if (s[s.size() - 1] != '\\')
					state = none;
				continue;
			}

			// Multi-line quote - only the last line is code
			if (state == quote) {
				if (s.find('"') == s.size() - 1) // String ends with "
					state = none;
				else
					continue;
			}

			// HERE documents
			if (state == heredoc) {
				std::string trimmed = trim_string(s, " \t\r\n`");

				if (trimmed == heredocMarker)
					state = none;
				continue;
			}

			if (s.find("$((") != std::string::npos || s.find("$[") != std::string::npos)
				arithmeticActive = true;

			if (s[s.size() - 1] == '\\') {
				state = backslash;
			} else if ((s.find("=\"") != std::string::npos || // Handle multi-line string assignments
					s.find("= \"") != std::string::npos) &&
					std::count(s.begin(), s.end(), '"') == 1) {
				state = quote;
				continue;
			} else {
				size_t heredocStart = s.find("<<");

				if (!arithmeticActive &&
						s.find("let ") != 0 &&
						s.find("$((") == std::string::npos &&
						s.find("))") == std::string::npos &&
						heredocStart != std::string::npos) {
					// Skip << and remove spaces before and after "EOF"
					heredocMarker = trim_string(s.substr(heredocStart + 2, s.size()));

					// Make sure the heredoc marker is a word
					for (unsigned int i = 0; i < heredocMarker.size(); i++) {
						if (heredocMarker[i] == ' ' || heredocMarker[i] == '\t') {
							heredocMarker = heredocMarker.substr(0, i);
							break;
						}
					}

					if (heredocMarker[0] == '-') {
						// '-' marks tab-suppression in heredoc
						heredocMarker = heredocMarker.substr(1);
					}

					if (heredocMarker.length() > 2
					&& (heredocMarker[0] == '"' || heredocMarker[0] == '\'')
					&&  heredocMarker[0] == heredocMarker[heredocMarker.length()-1]) {
						// remove enclosing in quotes
						heredocMarker = heredocMarker.substr(1, heredocMarker.length()-2);
					}

					if (heredocMarker.size() > 0 && heredocMarker[0] != '<')
						state = heredoc;


					std::string beforeHeredoc = trim_string(s.substr(0, heredocStart));
					/*
					 * Special case for things like
					 * while read -r line ; do
					 *     echo $line
					 * done <<EOF
					 * xxx
					 * yyy
					 * EOF
					 *
					 * where the done statement is nocode (as above)
					 */
					if (beforeHeredoc == "done")
						continue;
				}
			}

			if (s.find("case") == 0)
				caseActive = true;
			else if (s.find("esac") == 0)
				caseActive = false;

			if (s.find("))") != std::string::npos || s.find("]") != std::string::npos)
				arithmeticActive = false;

			// Only the last line of arithmetic is code
			if (arithmeticActive)
				continue;

			// Case switches are nocode
			if (caseActive && s[s.size() - 1] == ')') {
				// But let functions be (albeit not with balanced parentheses)
			        size_t startParenthesis = s.find("(");
				if (startParenthesis == std::string::npos || startParenthesis == 0)
					continue;
			}

			fileLineFound(crc, filename, lineNo);
		}
	}

	pid_t m_child;
	FILE *m_stderr;
	FILE *m_stdout;
	bool m_bashSupportsXtraceFd;
	enum InputType m_inputType;
};

// This ugly stuff should be fixed
static BashEngine *g_bashEngine;
class BashCtor
{
public:
	BashCtor()
	{
		g_bashEngine = new BashEngine();
	}
};
static BashCtor g_bashCtor;

class BashEngineCreator : public IEngineFactory::IEngineCreator
{
public:
	virtual ~BashEngineCreator()
	{
	}

	virtual IEngine *create(IFileParser &parser)
	{
		return g_bashEngine;
	}


	unsigned int matchFile(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		std::string s((const char *)data, 80);

		if (filename.size() >= 3 && filename.substr(filename.size() - 3, filename.size()) == ".sh")
			return 200;

		if (s.find("#!/bin/bash") != std::string::npos)
			return 400;
		if (s.find("#!/bin/sh") != std::string::npos)
			return 300;
		if (s.find("bash") != std::string::npos)
			return 100;
		if (s.find("sh") != std::string::npos)
			return 50;

		return match_none;
	}
};

static BashEngineCreator g_bashEngineCreator;
