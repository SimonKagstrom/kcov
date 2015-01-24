#include <file-parser.hh>
#include <engine.hh>
#include <configuration.hh>
#include <output-handler.hh>
#include <lineid.hh>
#include <utils.hh>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#include <list>
#include <unordered_map>
#include <vector>

#include <swap-endian.hh>

#include "script-engine-base.hh"

extern std::vector<uint8_t> bash_helper_data;

using namespace kcov;

class BashEngine : public ScriptEngineBase
{
public:
	BashEngine() :
		ScriptEngineBase(),
		m_child(0),
		m_stderr(NULL)
	{
	}

	~BashEngine()
	{
		kill(SIGTERM);
	}

	bool start(IEventListener &listener, const std::string &executable)
	{
		int stderrPipe[2];

		if (pipe(stderrPipe) < 0) {
			error("Failed to open pipe");

			return false;
		}

		std::string helperPath =
				IOutputHandler::getInstance().getBaseDirectory() + "bash-helper.sh";

		if (write_file(bash_helper_data.data(), bash_helper_data.size(),
				"%s", helperPath.c_str()) < 0) {
				error("Can't write helper at %s", helperPath.c_str());

				return false;
		}

		m_listener = &listener;

		/* Launch child */
		m_child = fork();
		if (m_child > 0) {

			// Close the parents write end of the pipe
			close(stderrPipe[1]);

			// And open a FILE * to stderr
			m_stderr = fdopen(stderrPipe[0], "r");
			if (!m_stderr) {
				error("Can't reopen the stderr pipe");
				return false;
			}

		} else if (m_child == 0) {
			IConfiguration &conf = IConfiguration::getInstance();
			const char **argv = conf.getArgv();
			unsigned int argc = conf.getArgc();

			const std::string command = conf.keyAsString("bash-command");

			/* Close the childs read end of the pipe */
			close(stderrPipe[0]);

			if (dup2(stderrPipe[1], 2) < 0) {
				perror("Failed to exchange stderr for pipe");
				return false;
			}

			/* Close the childs old write end of the pipe */
			close(stderrPipe[1]);

			/* Set up PS4 for tracing */
			doSetenv("PS4=kcov@${BASH_SOURCE}@${LINENO}@");
			doSetenv(fmt("BASH_ENV=%s", helperPath.c_str()));

			// Make a copy of the vector, now with "bash -x" first
			char **vec;
			vec = (char **)xmalloc(sizeof(char *) * (argc + 3));
			vec[0] = xstrdup(conf.keyAsString("bash-command").c_str());
			vec[1] = xstrdup("-x");
			for (unsigned i = 0; i < argc; i++)
				vec[2 + i] = xstrdup(argv[i]);

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

		return true;
	}

	bool checkEvents()
	{
		char *curLine;
		ssize_t len;
		size_t linecap = 0;

		len = getline(&curLine, &linecap, m_stderr);
		if (len < 0)
			return false;

		std::string cur(curLine);
		// Line markers always start with kcov@

		size_t kcovStr = cur.find("kcov@");
		if (kcovStr == std::string::npos) {
			fprintf(stderr, "%s", cur.c_str());
			return true;
		}
		std::vector<std::string> parts = split_string(cur.substr(kcovStr), "@");

		// kcov@FILENAME@LINENO@...
		if (parts.size() < 3)
			return true;

		// Resolve filename (might be relative)
		const std::string &filename = get_real_path(parts[1]);
		const std::string &lineNo = parts[2];

		if (!string_is_integer(lineNo)) {
			error("%s is not an integer", lineNo.c_str());

			return false;
		}

		if (!m_reportedFiles[filename]) {
			m_reportedFiles[filename] = true;

			for (FileListenerList_t::const_iterator it = m_fileListeners.begin();
					it != m_fileListeners.end();
					++it)
				(*it)->onFile(filename, IFileParser::FLG_NONE);

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
		if (rv != m_child)
			return true;

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

		if (filename.substr(filename.size() - 3, filename.size()) == ".sh")
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

		size_t sz;
		char *p = (char *)read_file(&sz, "%s", filename.c_str());

		// Can't handle this file
		if (!p)
			return;
		std::string fileData(p, sz);

		// Compute hash for this file
		uint32_t crc = hash_block((const uint8_t *)p, sz);

		free((void*)p);

		const std::vector<std::string> &stringList = split_string(fileData, "\n");
		unsigned int lineNo = 0;
		enum { none, backslash, heredoc } state = none;
		bool caseActive = false;
		std::string heredocMarker;

		for (std::vector<std::string>::const_iterator it = stringList.begin();
				it != stringList.end();
				++it) {
			std::string s = trim_string(*it);

			lineNo++;

			// Remove comments
			size_t comment = s.find("#");
			if (comment != std::string::npos) {
				s = s.substr(0, comment);
				s = trim_string(s);
			}

			// Empty line, ignore
			if (s == "")
				continue;

			if (s.size() >= 2 &&
					s[0] == ';' && s[1] == ';')
				continue;

			// Empty braces
			if (s[0] == '{' || s[0] == '}')
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
			// HERE documents
			if (state == heredoc) {
				if (s == heredocMarker)
					state = none;
				continue;
			}

			if (s[s.size() - 1] == '\\') {
				state = backslash;
			} else {
				size_t heredocStart = s.find("<<");

				if (heredocStart != std::string::npos) {
					// Skip << and remove spaces before and after "EOF"
					heredocMarker = trim_string(s.substr(heredocStart + 2, s.size()));

					// Make sure the heredoc marker is a word
					for (unsigned int i = 0; i < heredocMarker.size(); i++) {
						if (heredocMarker[i] == ' ' || heredocMarker[i] == '\t') {
							heredocMarker = heredocMarker.substr(0, i);
							break;
						}
					}

					if (heredocMarker.size() > 0 && heredocMarker[0] != '<')
						state = heredoc;
				}
			}

			if (s.find("case") == 0)
				caseActive = true;
			else if (s.find("esac") == 0)
				caseActive = false;

			// Case switches are nocode
			if (caseActive && s[s.size() - 1] == ')') {
				// But let functions be
				if (s.find("(") == std::string::npos)
					continue;
			}

			fileLineFound(crc, filename, lineNo);
		}
	}

	pid_t m_child;
	FILE *m_stderr;
};

// This ugly stuff should be fixed
static BashEngine *g_bashEngine;
class Ctor
{
public:
	Ctor()
	{
		g_bashEngine = new BashEngine();
	}
};
static Ctor g_bashCtor;

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

		if (filename.substr(filename.size() - 3, filename.size()) == ".sh")
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
