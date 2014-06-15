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

using namespace kcov;

class BashEngine : public IEngine, public IFileParser
{
public:
	BashEngine() :
		m_child(0),
		m_stderr(NULL),
		m_listener(NULL),
		m_currentAddress(1) // 0 is an invalid address
	{
		IEngineFactory::getInstance().registerEngine(*this);
		IParserManager::getInstance().registerParser(*this);
	}

	~BashEngine()
	{
		kill(SIGTERM);
	}

	// From IEngine
	int registerBreakpoint(unsigned long addr)
	{
		// No breakpoints
		return 0;
	}

	void setupAllBreakpoints()
	{
	}

	bool clearBreakpoint(int id)
	{
		return true;
	}

	bool start(IEventListener &listener, const std::string &executable)
	{
		int stderrPipe[2];

		if (pipe(stderrPipe) < 0) {
			error("Failed to open pipe");

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
			auto &conf = IConfiguration::getInstance();
			const char **argv = conf.getArgv();
			unsigned int argc = conf.getArgc();

			// Make a copy of the vector, now with "bash -x" first
			char **vec;
			vec = (char **)xmalloc(sizeof(char *) * (argc + 3));
			vec[0] = xstrdup("/bin/bash");
			vec[1] = xstrdup("-x");
			for (unsigned i = 0; i < argc; i++)
				vec[2 + i] = xstrdup(argv[i]);

			const std::string command = "/bin/bash";

			/* Close the childs read end of the pipe */
			close(stderrPipe[0]);

			if (dup2(stderrPipe[1], 2) < 0) {
				perror("Failed to exchange stderr for pipe");
				return false;
			}

			/* Close the childs old write end of the pipe */
			close(stderrPipe[1]);

			/* Set up PS4 for tracing */
			std::string ps4 = "PS4=kcov@${BASH_SOURCE}@${LINENO}@";

			char *envString = (char *)xmalloc(ps4.size() + 1);
			strcpy(envString, ps4.c_str());
			putenv(envString);

			/* Execute the script */
			if (execv(vec[0], vec)) {
				perror("Failed to execute script");

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

		auto kcovStr = cur.find("kcov@");
		if (kcovStr == std::string::npos)
			return true;
		auto parts = split_string(cur.substr(kcovStr), "@");

		// kcov@FILENAME@LINENO@...
		if (parts.size() < 3)
			return true;

		// Resolve filename (might be relative)
		const auto &filename = get_real_path(parts[1]);
		const auto &lineNo = parts[2];

		if (!string_is_integer(lineNo)) {
			error("%s is not an integer", lineNo.c_str());

			return false;
		}

		if (!m_reportedFiles[filename]) {
			m_reportedFiles[filename] = true;

			for (const auto &it : m_fileListeners)
				it->onFile(filename, IFileParser::FLG_NONE);

			parseFile(filename);
		}

		if (m_listener) {
			uint64_t address = 0;
			Event ev;

			auto it = m_lineIdToAddress.find(LineId(filename, string_to_integer(lineNo)));
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

	unsigned int matchFile(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		return matchParser(filename, data, dataSize);
	}


	// From IFileParser
	bool addFile(const std::string &filename, struct phdr_data_entry *phdr_data)
	{
		return true;
	}

	void registerLineListener(ILineListener &listener)
	{
		m_lineListeners.push_back(&listener);
	}

	void registerFileListener(IFileListener &listener)
	{
		m_fileListeners.push_back(&listener);
	}

	bool parse()
	{
		return true;
	}

	uint64_t getChecksum()
	{
		return 0;
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

	class Ctor
	{
	public:
		Ctor()
		{
			new BashEngine();
		}
	};

private:
	void reportEvent(enum event_type type, int data = -1, uint64_t address = 0)
	{
		if (!m_listener)
			return;

		m_listener->onEvent(Event(type, data, address));
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

		// Compute crc32 for this file
		uint32_t crc = crc32((const uint8_t *)p, sz);

		free((void*)p);

		const auto &stringList = split_string(fileData, "\n");
		unsigned int lineNo = 0;

		for (const auto &it : stringList) {
			const auto &s = trim_string(it);

			lineNo++;
			// Empty line, ignore
			if (s == "")
				continue;

			// Non-empty, but comment
			if (s[0] == '#')
				continue;

			// Empty braces
			if (s[0] == '{' || s[0] == '}')
				continue;

			// Functions
			if (s.find("function") == 0)
				continue;

			fileLineFound(crc, filename, lineNo);
		}
	}

	void fileLineFound(uint32_t crc, const std::string &filename, unsigned int lineNo)
	{
		LineId id(filename, lineNo);
		uint64_t address = m_currentAddress ^ crc;

		m_lineIdToAddress[id] = address;

		for (const auto &lit : m_lineListeners)
			lit->onLine(filename.c_str(), lineNo, address);

		m_currentAddress++;
	}

	typedef std::list<ILineListener *> LineListenerList_t;
	typedef std::list<IFileListener *> FileListenerList_t;
	typedef std::unordered_map<std::string, bool> ReportedFileMap_t;
	typedef std::unordered_map<LineId, uint64_t, LineIdHash> LineIdToAddressMap_t;

	pid_t m_child;
	FILE *m_stderr;

	LineListenerList_t m_lineListeners;
	FileListenerList_t m_fileListeners;
	ReportedFileMap_t m_reportedFiles;
	LineIdToAddressMap_t m_lineIdToAddress;

	IEventListener *m_listener;
	uint64_t m_currentAddress;
};

static BashEngine::Ctor g_bashEngine;
