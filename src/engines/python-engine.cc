#include <file-parser.hh>
#include <engine.hh>

using namespace kcov;

class PythonEngine : public IEngine, public IFileParser
{
public:
	PythonEngine()
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
		return true;
	}

	const Event waitEvent()
	{
		Event out;

		return out;
	}

	void continueExecution(const Event ev)
	{
	}

	bool childrenLeft()
	{
		return true;
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
		return match_none;
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
		return match_none;
	}
};

static PythonEngine g_instance;
