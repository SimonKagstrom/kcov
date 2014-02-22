#include <file-parser.hh>
#include <engine.hh>
#include <configuration.hh>
#include <output-handler.hh>
#include <utils.hh>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <list>
#include <unordered_map>

#include "../swap-endian.hh"

using namespace kcov;

extern "C" size_t python_helper_data_size;
extern "C" uint8_t python_helper_data[];


const uint64_t COVERAGE_MAGIC = 0x6d6574616c6c6775ULL; // "metallgut"

/* Should be 8-byte aligned */
struct coverage_data
{
	uint64_t magic;
	uint32_t size;
	uint32_t line;
	const char filename[];
};

class PythonEngine : public IEngine, public IFileParser
{
public:
	PythonEngine() :
		m_child(0),
		m_running(false)
	{
		IEngineFactory::getInstance().registerEngine(*this);
		IParserManager::getInstance().registerParser(*this);
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

	bool start(const std::string &executable)
	{
		std::string kcov_python_pipe_path =
				IOutputHandler::getInstance().getOutDirectory() + "kcov-python.pipe";
		std::string kcov_python_path =
				IOutputHandler::getInstance().getBaseDirectory() + "python-helper.py";

		if (write_file(python_helper_data, python_helper_data_size, kcov_python_path.c_str()) < 0) {
				error("Can't write python helper at %s", kcov_python_path.c_str());

				return false;
		}

		std::string kcov_python_env = "KCOV_PYTHON_PIPE_PATH=" + kcov_python_pipe_path;
		unlink(kcov_python_pipe_path.c_str());
		mkfifo(kcov_python_pipe_path.c_str(), 0600);

		char *envString = (char *)xmalloc(kcov_python_env.size() + 1);
		strcpy(envString, kcov_python_env.c_str());

		putenv(envString);

		/* Launch the python helper */
		m_child = fork();
		if (m_child == 0) {
			const char **argv = IConfiguration::getInstance().getArgv();
			unsigned int argc = IConfiguration::getInstance().getArgc();

			std::string s = fmt("python %s ", kcov_python_path.c_str());
			for (unsigned int i = 0; i < argc; i++)
				s += std::string(argv[i]) + " ";

			int res;

			res = system(s.c_str());
			panic_if (res < 0,
					"Can't execute python helper");

			exit(WEXITSTATUS(res));
		} else if (m_child < 0) {
			perror("fork");

			return false;
		}
		m_running = true;
		m_pipe = fopen(kcov_python_pipe_path.c_str(), "r");
		panic_if (!m_pipe,
				"Can't open python pipe %s", kcov_python_pipe_path.c_str());

		return true;
	}

	const Event waitEvent()
	{
		uint8_t buf[8192];
		Event out;
		int status;
		int rv;

		rv = fread((void *)buf, 1, sizeof(buf), m_pipe);
		if (rv < (int)sizeof(struct coverage_data)) {
			error("Read too little");

			m_running = false;
			out.type = ev_error;

			return out;
		}
		if (parseCoverageData(buf, rv) != true) {
			m_running = false;
			out.type = ev_error;

			return out;
		}

		rv = waitpid(m_child, &status, 0);
		if (rv != m_child)
			return out;

		if (WIFEXITED(status)) {
			out.type = ev_exit_first_process;
			out.data = WEXITSTATUS(status);
		} else {
			warning("Other status: 0x%x\n", status);
			out.type = ev_error;
		}

		return out;
	}

	void continueExecution(const Event ev)
	{
	}

	bool childrenLeft()
	{
		return m_running;
	}

	std::string eventToName(Event ev)
	{
		return "";
	}

	void kill()
	{
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

		if (filename.substr(filename.size() - 3, filename.size()) == ".py")
			return 200;

		if (s.find("python") != std::string::npos)
			return 100;

		return match_none;
	}
private:

	void unmarshalCoverageData(struct coverage_data *p)
	{
		p->magic = be_to_host<uint64_t>(p->magic);
		p->size = be_to_host<uint32_t>(p->size);
		p->line = be_to_host<uint32_t>(p->line);
	}

	bool parseCoverageData(uint8_t *raw, size_t size)
	{
		size_t size_left = size;

		while (size_left > 0) {
			struct coverage_data *p = (struct coverage_data *)raw;

			unmarshalCoverageData(p);
			if (p->magic != COVERAGE_MAGIC ||
					p->size > size_left) {
				error("Data magic wrong or size too large: magic 0x%llx, size %u (%zu left)\n",
						(unsigned long long)p->magic,
						(unsigned int)p->size,
						size_left);

				return false;
			}

			if (!m_reportedFiles[p->filename]) {
				for (const auto &it : m_fileListeners)
					it->onFile(p->filename, IFileParser::FLG_NONE);
			}
			for (const auto &it : m_lineListeners)
				it->onLine(p->filename, p->line, 1);

			printf("CUR: %s:%u\n", p->filename, p->line);

			raw += p->size;
			size_left -= p->size;
		}

		return true;
	}


	typedef std::list<ILineListener *> LineListenerList_t;
	typedef std::list<IFileListener *> FileListenerList_t;
	typedef std::unordered_map<std::string, bool> ReportedFileMap_t;

	pid_t m_child;
	bool m_running;
	FILE *m_pipe;

	LineListenerList_t m_lineListeners;
	FileListenerList_t m_fileListeners;
	ReportedFileMap_t m_reportedFiles;
};

static PythonEngine g_instance;
