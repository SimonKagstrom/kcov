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

		std::string kcov_python_env = "KCOV_SOLIB_PATH=" + kcov_python_pipe_path;
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

		return true;
	}

	const Event waitEvent()
	{
		Event out;
		int status;
		int rv;

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
	}

	void registerFileListener(IFileListener &listener)
	{
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
		pid_t m_child;
		bool m_running;
};

static PythonEngine g_instance;
