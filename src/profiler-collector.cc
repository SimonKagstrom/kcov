#include <collector.hh>
#include <elf.hh>
#include <utils.hh>

#include <unordered_map>
#include <string>
#include <list>

using namespace kcov;

class Collector : public ICollector
{
public:
	Collector(IElf &elf) : m_elf(elf)
	{
	}

	void registerListener(ICollector::IListener &listener)
	{
		m_listeners.push_back(&listener);
	}

	int run()
	{
		size_t sz;
		uint32_t *data;

		data = (uint32_t *)read_file(&sz, "/tmp/addresses");
		if (!data) {
			error("Can't read address file\n");
			return 1;
		}

		// This will read all addresses -> lines
		m_elf.parse();

		// FIXME: Here are assumptions that won't hold...
		unsigned addrs = sz / (sizeof(uint32_t) * 2);
		for (unsigned i = 0; i < addrs; i += 2) {
			uint32_t addr = data[i];
			uint32_t hits = data[i + 1];

			// Report addresses
			for (ListenerList_t::iterator it = m_listeners.begin();
					it != m_listeners.end();
					it++) {
				(*it)->onAddress(addr, hits);
			}
		}

		free(data);

		return 0;
	}

	virtual void stop()
	{
	}

private:
	typedef std::list<ICollector::IListener *> ListenerList_t;

	IElf &m_elf;
	ListenerList_t m_listeners;
};

ICollector &ICollector::create(IElf *elf)
{
	return *new Collector(*elf);
}
