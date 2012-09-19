#include <reporter.hh>
#include <elf.hh>
#include <collector.hh>
#include <utils.hh>
#include <filter.hh>

#include <string>
#include <list>
#include <unordered_map>
#include <mutex>

using namespace kcov;

#define KCOV_MAGIC      0x6b636f76 /* "kcov" */
#define KCOV_DB_VERSION 1

struct marshalHeaderStruct
{
	uint32_t magic;
	uint32_t db_version;
	uint64_t checksum;
};

class Reporter : public IReporter, public IElf::ILineListener, public ICollector::IListener
{
public:
	Reporter(IElf &elf, ICollector &collector) :
		m_elf(elf), m_collector(collector), m_filter(IFilter::getInstance())
	{
		m_elf.registerLineListener(*this);
		m_collector.registerListener(*this);
	}

	bool lineIsCode(const char *file, unsigned int lineNr)
	{
		bool out;

		m_mutex.lock();
		out =  m_lines.find(LineId(file, lineNr)) != m_lines.end();
		m_mutex.unlock();

		return out;
	}

	LineExecutionCount getLineExecutionCount(const char *file, unsigned int lineNr)
	{
		unsigned int hits = 0;
		unsigned int possibleHits = 0;

		m_mutex.lock();
		LineMap_t::iterator it = m_lines.find(LineId(file, lineNr));

		if (it != m_lines.end()) {
			Line *line = it->second;

			hits = line->hits();
			possibleHits = line->possibleHits();
		}
		m_mutex.unlock();

		return LineExecutionCount(hits,
				possibleHits);
	}

	ExecutionSummary getExecutionSummary()
	{
		unsigned int executedLines = 0;
		unsigned int nrLines = 0;

		m_mutex.lock();
		for (LineMap_t::iterator it = m_lines.begin();
				it != m_lines.end();
				it++) {
			Line *cur = it->second;

			if (!m_filter.runFilters(cur->m_file))
				continue;

			executedLines += !!cur->hits();
			nrLines++;
		}
		m_mutex.unlock();

		return ExecutionSummary(nrLines, executedLines);
	}

	void *marshal(size_t *szOut)
	{
		size_t sz = getMarshalSize();
		void *start;
		uint8_t *p;

		start = malloc(sz);
		if (!start)
			return NULL;
		memset(start, 0, sz);
		p = marshalHeader((uint8_t *)start);

		m_mutex.lock();
		for (LineMap_t::iterator it = m_lines.begin();
				it != m_lines.end();
				it++) {
			Line *cur = it->second;

			p = cur->marshal(p);
		}
		m_mutex.unlock();

		*szOut = sz;

		return start;
	}

	bool unMarshal(void *data, size_t sz)
	{
		uint8_t *start = (uint8_t *)data;
		uint8_t *p = start;
		size_t n;

		p = unMarshalHeader(p);

		if (!p)
			return false;

		n = (sz - (p - start)) / getMarshalEntrySize();

		m_mutex.lock();
		// Should already be 0, but anyway
		for (AddrToLineMap_t::iterator it = m_addrToLine.begin();
				it != m_addrToLine.end();
				it++)
			it->second->clearHits();

		for (size_t i = 0; i < n; i++) {
			unsigned long addr;
			unsigned int hits;

			p = Line::unMarshal(p, &addr, &hits);
			Line *line = m_addrToLine[addr];

			if (!line)
				continue;

			if (!hits)
				continue;

			// Really an internal error, but let's not hang on corrupted data
			if (hits > line->possibleHits())
				hits = line->possibleHits();

			// Register all hits for this address
			while (hits--)
				line->registerHit(addr);
		}
		m_mutex.unlock();

		return true;
	}

	virtual void stop()
	{
		// Otherwise the writer thread can hang here
		m_mutex.unlock();
	}


private:
	size_t getMarshalEntrySize()
	{
		return 2 * sizeof(uint64_t);
	}

	size_t getMarshalSize()
	{
		size_t out = 0;

		for (LineMap_t::iterator it = m_lines.begin();
				it != m_lines.end();
				it++) {
			Line *cur = it->second;

			out += cur->m_addrs.size();
		}


		return out * getMarshalEntrySize() + sizeof(struct marshalHeaderStruct);
	}

	uint8_t *marshalHeader(uint8_t *p)
	{
		struct marshalHeaderStruct *hdr = (struct marshalHeaderStruct *)p;

		hdr->magic = KCOV_MAGIC;
		hdr->db_version = KCOV_DB_VERSION;
		hdr->checksum = m_elf.getChecksum();

		return p + sizeof(struct marshalHeaderStruct);
	}

	uint8_t *unMarshalHeader(uint8_t *p)
	{
		struct marshalHeaderStruct *hdr = (struct marshalHeaderStruct *)p;

		if (hdr->magic != KCOV_MAGIC)
			return NULL;

		if (hdr->db_version != KCOV_DB_VERSION)
			return NULL;

		if (hdr->checksum != m_elf.getChecksum())
			return NULL;

		return p + sizeof(struct marshalHeaderStruct);
	}

	/* Called when the ELF is parsed */
	void onLine(const char *file, unsigned int lineNr, unsigned long addr)
	{
		LineId key(file, lineNr);

		m_mutex.lock();
		LineMap_t::iterator it = m_lines.find(key);
		Line *line;

		if (it == m_lines.end()) {
			if (!file_exists(file))
				goto out;

			line = new Line(key);
			m_lines[key] = line;
		} else {
			line = it->second;
		}

		line->addAddress(addr);
		m_addrToLine[addr] = line;

out:
		m_mutex.unlock();
	}

	/* Called during runtime */
	void onAddress(unsigned long addr)
	{
		m_mutex.lock();
		AddrToLineMap_t::iterator it = m_addrToLine.find(addr);

		if (it != m_addrToLine.end()) {
			Line *line = it->second;

			line->registerHit(addr);
		}
		m_mutex.unlock();
	}


	class LineId
	{
	public:
		LineId(const char *fileName, int nr) :
			m_file(fileName), m_lineNr(nr)
		{
		}

		bool operator==(const LineId &other) const
		{
			return (m_file == other.m_file) && (m_lineNr == other.m_lineNr);
		}

		std::string m_file;
		unsigned int m_lineNr;
	};

	class LineIdHash
	{
	public:
		size_t operator()(const LineId &obj) const
		{
			return std::hash<std::string>()(obj.m_file) ^ std::hash<int>()(obj.m_lineNr);
		}
	};

	class Line
	{
	public:
		typedef std::unordered_map<unsigned long, int> AddrToHitsMap_t;

		Line(LineId id) : m_file(id.m_file),
				m_lineNr(id.m_lineNr)

		{
		}

		void addAddress(unsigned long addr)
		{
			m_addrs[addr] = 0;
		}

		unsigned int registerHit(unsigned long addr)
		{
			unsigned int out = !m_addrs[addr];

			m_addrs[addr] = 1;

			return out;
		}

		void clearHits()
		{
			for (AddrToHitsMap_t::iterator it = m_addrs.begin();
					it != m_addrs.end();
					it++)
				it->second = 0;
		}

		unsigned int hits()
		{
			unsigned int out = 0;

			for (AddrToHitsMap_t::iterator it = m_addrs.begin();
					it != m_addrs.end();
					it++)
				out += it->second;

			return out;
		}

		unsigned int possibleHits()
		{
			return m_addrs.size();
		}

		uint8_t *marshal(uint8_t *start)
		{
			uint64_t *data = (uint64_t *)start;

			for (AddrToHitsMap_t::iterator it = m_addrs.begin();
					it != m_addrs.end();
					it++) {
				// Address and number of hits
				*data++ = (uint64_t)it->first;
				*data++ = (uint64_t)it->second;
			}

			return (uint8_t *)data;
		}

		static uint8_t *unMarshal(uint8_t *p,
				unsigned long *outAddr, unsigned int *outHits)
		{
			uint64_t *data = (uint64_t *)p;

			*outAddr = *data++;
			*outHits = *data++;

			return (uint8_t *)data;
		}

		std::string m_file;
		unsigned int m_lineNr;
		AddrToHitsMap_t m_addrs;
	};

	typedef std::unordered_map<LineId, Line *, LineIdHash> LineMap_t;
	typedef std::unordered_map<unsigned long, Line *> AddrToLineMap_t;

	LineMap_t m_lines;
	AddrToLineMap_t m_addrToLine;
	std::mutex m_mutex; // Protects m_lines, m_addrToLine

	IElf &m_elf;
	ICollector &m_collector;
	IFilter &m_filter;
};

IReporter &IReporter::create(IElf &elf, ICollector &collector)
{
	return *new Reporter(elf, collector);
}
