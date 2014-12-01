#include <reporter.hh>
#include <file-parser.hh>
#include <collector.hh>
#include <utils.hh>
#include <filter.hh>
#include <lineid.hh>

#include <string>
#include <list>
#include <unordered_map>

#include "swap-endian.hh"

using namespace kcov;

#define KCOV_MAGIC      0x6b636f76 /* "kcov" */
#define KCOV_DB_VERSION 2

struct marshalHeaderStruct
{
	uint32_t magic;
	uint32_t db_version;
	uint64_t checksum;
};

class Reporter :
		public IReporter,
		public IFileParser::ILineListener,
		public ICollector::IListener,
		public IReporter::IListener
{
public:
	Reporter(IFileParser &fileParser, ICollector &collector, IFilter &filter) :
		m_fileParser(fileParser), m_collector(collector), m_filter(filter)
	{
		m_fileParser.registerLineListener(*this);
		m_collector.registerListener(*this);
	}

	~Reporter()
	{
		stop();

		for (LineMap_t::iterator it = m_lines.begin();
				it != m_lines.end();
				++it)
			free(it->second);
	}

	void registerListener(IReporter::IListener &listener)
	{
		m_listeners.push_back(&listener);
	}

	bool fileIsIncluded(const std::string &file)
	{
		return m_filter.runFilters(file);
	}

	bool lineIsCode(const std::string &file, unsigned int lineNr)
	{
		bool out;

		out = m_lines.find(getLineId(file, lineNr)) != m_lines.end();

		return out;
	}

	LineExecutionCount getLineExecutionCount(const std::string &file, unsigned int lineNr)
	{
		unsigned int hits = 0;
		unsigned int possibleHits = 0;

		LineMap_t::iterator it = m_lines.find(getLineId(file, lineNr));

		if (it != m_lines.end()) {
			Line *line = it->second;

			hits = line->hits();
			possibleHits = line->possibleHits();
		}

		return LineExecutionCount(hits,
				possibleHits);
	}

	ExecutionSummary getExecutionSummary()
	{
		unsigned int executedLines = 0;
		unsigned int nrLines = 0;

		for (LineMap_t::const_iterator it = m_lines.begin();
				it != m_lines.end();
				++it) {
			Line *cur = it->second;

			// Don't include non-existing files in summary
			if (!file_exists(cur->m_file))
				continue;

			if (!m_filter.runFilters(cur->m_file))
				continue;

			executedLines += !!cur->hits();
			nrLines++;
		}

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

		for (LineMap_t::const_iterator it = m_lines.begin();
				it != m_lines.end();
				++it) {
			Line *cur = it->second;

			p = cur->marshal(p);
		}

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

		for (size_t i = 0; i < n; i++) {
			unsigned long addr;
			unsigned int hits;

			p = Line::unMarshal(p, &addr, &hits);
			AddrToLineMap_t::iterator it = m_addrToLine.find(addr);

			if (!hits)
				continue;

			/* Can't find this address.
			 *
			 * Either it doesn't exist in the binary, or it hasn't been parsed
			 * yet, which will be the case for bash/python. Add it to pending
			 * addresses if so.
			 */
			if (it == m_addrToLine.end()) {
				m_pendingHits[addr] = hits;
				continue;
			}

			Line *line = it->second;

			// Really an internal error, but let's not hang on corrupted data
			if (hits > line->possibleHits())
				hits = line->possibleHits();

			// Register all hits for this address
			reportAddress(addr, hits);
		}

		return true;
	}

	virtual void stop()
	{
	}


private:
	size_t getMarshalEntrySize()
	{
		return 2 * sizeof(uint64_t);
	}

	size_t getMarshalSize()
	{
		size_t out = 0;

		for (LineMap_t::const_iterator it = m_lines.begin();
				it != m_lines.end();
				++it) {
			Line *cur = it->second;

			out += cur->m_addrs.size();
		}


		return out * getMarshalEntrySize() + sizeof(struct marshalHeaderStruct);
	}

	uint8_t *marshalHeader(uint8_t *p)
	{
		struct marshalHeaderStruct *hdr = (struct marshalHeaderStruct *)p;

		hdr->magic = to_be<uint32_t>(KCOV_MAGIC);
		hdr->db_version = to_be<uint32_t>(KCOV_DB_VERSION);
		hdr->checksum = to_be<uint64_t>(m_fileParser.getChecksum());

		return p + sizeof(struct marshalHeaderStruct);
	}

	uint8_t *unMarshalHeader(uint8_t *p)
	{
		struct marshalHeaderStruct *hdr = (struct marshalHeaderStruct *)p;

		if (be_to_host<uint32_t>(hdr->magic) != KCOV_MAGIC)
			return NULL;

		if (be_to_host<uint32_t>(hdr->db_version) != KCOV_DB_VERSION)
			return NULL;

		if (be_to_host<uint64_t>(hdr->checksum) != m_fileParser.getChecksum())
			return NULL;

		return p + sizeof(struct marshalHeaderStruct);
	}

	/* Called when the file is parsed */
	void onLine(const std::string &file, unsigned int lineNr, unsigned long addr)
	{
		if (!m_filter.runFilters(file))
			return;

		kcov_debug(INFO_MSG, "REPORT %s:%u at 0x%lx\n",
				file.c_str(), lineNr, (unsigned long)addr);
		size_t key = getLineId(file, lineNr);

		LineMap_t::iterator it = m_lines.find(key);
		Line *line;

		if (it == m_lines.end()) {
			line = new Line(file, lineNr);

			m_lines[key] = line;

		} else {
			line = it->second;
		}

		line->addAddress(addr);
		m_addrToLine[addr] = line;

		/*
		 * If we have pending hits for this address, service
		 * them here.
		 */
		AddrToHitsMap_t::iterator pend = m_pendingHits.find(addr);
		if (pend != m_pendingHits.end()) {
			reportAddress(addr, pend->second);

			m_pendingHits.erase(addr);
		}
	}

	/* Called during runtime */
	void reportAddress(unsigned long addr, unsigned long hits)
	{
		AddrToLineMap_t::iterator it = m_addrToLine.find(addr);

		if (it == m_addrToLine.end())
			return;

		Line *line = it->second;

		kcov_debug(INFO_MSG, "REPORT hit at 0x%lx\n", addr);
		line->registerHit(addr, hits);

		for (ListenerList_t::const_iterator it = m_listeners.begin();
				it != m_listeners.end();
				++it)
			(*it)->onAddress(addr, hits);
	}

	// From ICollector::IListener
	void onAddressHit(unsigned long addr, unsigned long hits)
	{
		reportAddress(addr, hits);
	}

	// From IReporter::IListener - report recursively
	void onAddress(unsigned long addr, unsigned long hits)
	{
	}

	class Line
	{
	public:
		typedef std::unordered_map<unsigned long, int> AddrToHitsMap_t;

		Line(const std::string &file, unsigned int lineNr) :
			m_file(file),
			m_lineNr(lineNr)

		{
		}

		void addAddress(unsigned long addr)
		{
			if (m_addrs.find(addr) == m_addrs.end())
				m_addrs[addr] = 0;
		}

		unsigned int registerHit(unsigned long addr, unsigned long hits)
		{
			unsigned int out = !m_addrs[addr];

			m_addrs[addr] = 1;

			return out;
		}

		void clearHits()
		{
			for (AddrToHitsMap_t::iterator it = m_addrs.begin();
					it != m_addrs.end();
					++it)
				it->second = 0;
		}

		unsigned int hits()
		{
			unsigned int out = 0;

			for (AddrToHitsMap_t::const_iterator it = m_addrs.begin();
					it != m_addrs.end();
					++it)
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

			for (AddrToHitsMap_t::const_iterator it = m_addrs.begin();
					it != m_addrs.end();
					++it) {
				// Address and number of hits
				*data++ = to_be<uint64_t>(it->first);
				*data++ = to_be<uint64_t>(it->second);
			}

			return (uint8_t *)data;
		}

		static uint8_t *unMarshal(uint8_t *p,
				unsigned long *outAddr, unsigned int *outHits)
		{
			uint64_t *data = (uint64_t *)p;

			*outAddr = be_to_host(*data++);
			*outHits = be_to_host(*data++);

			return (uint8_t *)data;
		}

		std::string m_file;
		unsigned int m_lineNr;
		AddrToHitsMap_t m_addrs;
	};

	typedef std::unordered_map<size_t, Line *> LineMap_t;
	typedef std::unordered_map<unsigned long, Line *> AddrToLineMap_t;
	typedef std::unordered_map<unsigned long, unsigned long> AddrToHitsMap_t;
	typedef std::vector<IReporter::IListener *> ListenerList_t;

	LineMap_t m_lines;
	AddrToLineMap_t m_addrToLine;
	AddrToHitsMap_t m_pendingHits;
	ListenerList_t m_listeners;

	IFileParser &m_fileParser;
	ICollector &m_collector;
	IFilter &m_filter;
};

IReporter &IReporter::create(IFileParser &parser, ICollector &collector, IFilter &filter)
{
	return *new Reporter(parser, collector, filter);
}
