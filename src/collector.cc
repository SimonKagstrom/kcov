#include <collector.hh>
#include <file-parser.hh>
#include <utils.hh>
#include <engine.hh>
#include <output-handler.hh>
#include <configuration.hh>

#include <unordered_map>
#include <string>
#include <list>

using namespace kcov;

class Collector : public ICollector, public IFileParser::ILineListener
{
public:
	Collector(IFileParser &fileParser) : m_fileParser(fileParser), m_engine(IEngine::getInstance())
	{
		m_fileParser.registerLineListener(*this);
	}

	void registerListener(ICollector::IListener &listener)
	{
		m_listeners.push_back(&listener);
	}

	int run(const std::string &filename)
	{
		int out = 0;

		if (!m_engine.start(filename)) {
			error("Can't start/attach to %s", filename.c_str());
			return -1;
		}

		IOutputHandler &output = IOutputHandler::getInstance();
		unsigned int outputInterval = IConfiguration::getInstance().getOutputInterval();

		// This will set all breakpoints
		m_fileParser.parse();
		m_engine.setupAllBreakpoints();

		uint64_t lastTimestamp = get_ms_timestamp();

		IEngine::Event ev;
		while (1) {
			m_engine.continueExecution(ev);

			ev = m_engine.waitEvent();
			switch (ev.type)
			{
			case ev_error:
				if (!m_engine.childrenLeft())
					goto out_err;
				break;
			case ev_signal:
				if (m_engine.childrenLeft())
					continue;
				kcov_debug(STATUS_MSG, "kcov: Process exited with signal %d (%s)\n", ev.data, m_engine.eventToName(ev).c_str());

				return -1;

			case ev_exit_first_process:
				out = ev.data;
				if (IConfiguration::getInstance().getExitFirstProcess()) {
					IConfiguration &conf = IConfiguration::getInstance();
					std::string fifoName = conf.getOutDirectory() + conf.getBinaryName() + "/done.fifo";

					std::string exitCode = fmt("%u", out);

					write_file(exitCode.c_str(), exitCode.size(), "%s", fifoName.c_str());
				}
			case ev_exit:
				out = ev.data;
				break;
			case ev_breakpoint:
				for (const auto &it : m_listeners)
					it->onAddress(ev.addr, 1);

				// Disable this breakpoint
				m_engine.clearBreakpoint(ev.data);

				break;

			default:
				error("Unknown event %d", ev.type);
				return -1;
			}

			uint64_t now = get_ms_timestamp();

			if (outputInterval != 0 &&
					now - lastTimestamp >= outputInterval) {
				lastTimestamp = now;

				output.produce();
			}
		}
out_err:
		return out;
	}

	virtual void stop()
	{
	}

private:
	void onLine(const char *file, unsigned int lineNr, unsigned long addr)
	{
		if (addr == 0)
			return;

		m_engine.registerBreakpoint(addr);
		m_addrs[addr]++;
	}

	typedef std::unordered_map<unsigned long, int> AddrMap_t;
	typedef std::list<ICollector::IListener *> ListenerList_t;

	AddrMap_t m_addrs;
	IFileParser &m_fileParser;
	IEngine &m_engine;
	ListenerList_t m_listeners;
};

ICollector &ICollector::create(IFileParser *elf)
{
	return *new Collector(*elf);
}
