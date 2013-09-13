#include <collector.hh>
#include <elf.hh>
#include <utils.hh>
#include <engine.hh>
#include <output-handler.hh>
#include <configuration.hh>

#include <unordered_map>
#include <string>
#include <list>

using namespace kcov;

class Collector : public ICollector, public IElf::ILineListener
{
public:
	Collector(IElf &elf) : m_elf(elf), m_engine(IEngine::getInstance())
	{
		m_elf.registerLineListener(*this);
	}

	void registerListener(ICollector::IListener &listener)
	{
		m_listeners.push_back(&listener);
	}

	int run()
	{
		if (!m_engine.start(m_elf.getFilename())) {
			error("Can't start/attach to %s", m_elf.getFilename());
			return -1;
		}

		IOutputHandler &output = IOutputHandler::getInstance();
		unsigned int outputInterval = IConfiguration::getInstance().getOutputInterval();

		// This will set all breakpoints
		m_elf.parse();
		m_engine.setupAllBreakpoints();

		uint64_t lastTimestamp = get_ms_timestamp();

		while (1) {
			IEngine::Event ev;

			ev = m_engine.continueExecution();
			switch (ev.type)
			{
			case ev_error:
			case ev_signal:
				fprintf(stderr, "Process exited with %s\n", m_engine.eventToName(ev).c_str());

				return -1;

			case ev_exit:
				return ev.data;

			case ev_breakpoint:
				for (ListenerList_t::iterator it = m_listeners.begin();
						it != m_listeners.end();
						it++)
					(*it)->onAddress(ev.addr, 1);

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

		/* Should never get here */
		panic("Unreachable code\n");

		return 0;
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
	IElf &m_elf;
	IEngine &m_engine;
	ListenerList_t m_listeners;
};

ICollector &ICollector::create(IElf *elf)
{
	return *new Collector(*elf);
}
