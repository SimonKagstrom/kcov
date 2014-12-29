#include <reporter.hh>
#include <file-parser.hh>
#include <collector.hh>
#include <utils.hh>
#include <filter.hh>

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

		m_maxPossibleHits = fileParser.maxPossibleHits();
	}

	~Reporter()
	{
		stop();
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
		// Not code if the file doesn't exist!
		if (m_files.find(file) == m_files.end())
			return false;

		return m_files[file].lineIsCode(lineNr);
	}

	LineExecutionCount getLineExecutionCount(const std::string &file, unsigned int lineNr)
	{
		unsigned int hits = 0;
		unsigned int possibleHits = 0;

		FileMap_t::const_iterator it = m_files.find(file);

		if (it != m_files.end()) {
			const Line *line = it->second.getLine(lineNr);

			if (line) {
				hits = line->hits();
				possibleHits = line->possibleHits(m_maxPossibleHits != IFileParser::HITS_UNLIMITED);
			}
		}

		return LineExecutionCount(hits, possibleHits);
	}

	ExecutionSummary getExecutionSummary()
	{
		unsigned int executedLines = 0;
		unsigned int nrLines = 0;

		// Summarize all files
		for (FileMap_t::const_iterator it = m_files.begin();
				it != m_files.end();
				++it) {
			const std::string &fileName = it->first;
			const File &file = it->second;

			// Don't include non-existing files in summary
			if (!file_exists(fileName))
				continue;

			// And not filtered ones either
			if (!m_filter.runFilters(fileName))
				continue;

			executedLines += file.getExecutedLines();
			nrLines += file.getNrLines();
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

		// Marshal all lines in the files
		for (FileMap_t::const_iterator it = m_files.begin();
				it != m_files.end();
				++it) {
			const File &cur = it->second;

			p = cur.marshal(p);
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
			if (m_maxPossibleHits != IFileParser::HITS_UNLIMITED && hits > line->possibleHits(true))
				hits = line->possibleHits(true);

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

		// Sum all file sizes
		for (FileMap_t::const_iterator it = m_files.begin();
				it != m_files.end();
				++it) {
			out += it->second.marshalSize();
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

		Line *line = m_files[file].getLine(lineNr);

		if (!line) {
			line = new Line(file, lineNr);
			m_files[file].addLine(lineNr, line);
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
		line->registerHit(addr, hits, m_maxPossibleHits != IFileParser::HITS_UNLIMITED);

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

		Line(const std::string &file, unsigned int lineNr)
		{
		}

		void addAddress(unsigned long addr)
		{
			if (m_addrs.find(addr) == m_addrs.end())
				m_addrs[addr] = 0;
		}

		void registerHit(unsigned long addr, unsigned long hits, bool singleShot)
		{
			if (singleShot)
				m_addrs[addr] = 1;
			else
				m_addrs[addr] += hits;
		}

		void clearHits()
		{
			for (AddrToHitsMap_t::iterator it = m_addrs.begin();
					it != m_addrs.end();
					++it)
				it->second = 0;
		}

		unsigned int hits() const
		{
			unsigned int out = 0;

			for (AddrToHitsMap_t::const_iterator it = m_addrs.begin();
					it != m_addrs.end();
					++it)
				out += it->second;

			return out;
		}

		unsigned int possibleHits(bool singleShot) const
		{
			if (singleShot)
				return m_addrs.size();

			return 0; // Meaning any number of hits are possible
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

		size_t marshalSize() const
		{
			return m_addrs.size();
		}

		static uint8_t *unMarshal(uint8_t *p,
				unsigned long *outAddr, unsigned int *outHits)
		{
			uint64_t *data = (uint64_t *)p;

			*outAddr = be_to_host(*data++);
			*outHits = be_to_host(*data++);

			return (uint8_t *)data;
		}

	private:
		AddrToHitsMap_t m_addrs;
	};

	class File
	{
	public:
		File() : m_nrLines(0)
		{
		}

		~File()
		{
			for (unsigned int i = 0; i < m_lines.size(); i++) {
				Line *cur = m_lines[i];

				if (!cur)
					continue;

				delete cur;
			}
		}

		void addLine(unsigned int lineNr, Line *line)
		{
			// Resize the vector to fit this line
			if (lineNr >= m_lines.size())
				m_lines.resize(lineNr + 1);

			m_lines[lineNr] = line;
			m_nrLines++;
		}

		Line *getLine(unsigned int lineNr) const
		{
			if (lineNr >= m_lines.size())
				return NULL;

			return m_lines[lineNr];
		}

		// Marshal all line data
		uint8_t *marshal(uint8_t *p) const
		{
			for (unsigned int i = 0; i < m_lines.size(); i++) {
				Line *cur = m_lines[i];

				if (!cur)
					continue;

				p = cur->marshal(p);
			}

			return p;
		}

		size_t marshalSize() const
		{
			size_t out = 0;

			for (unsigned int i = 0; i < m_lines.size(); i++) {
				Line *cur = m_lines[i];

				if (!cur)
					continue;

				out += cur->marshalSize();
			}

			return out;
		}

		unsigned int getExecutedLines() const
		{
			unsigned int out = 0;

			for (unsigned int i = 0; i < m_lines.size(); i++) {
				Line *cur = m_lines[i];

				if (!cur)
					continue;

				// Hits as zero or one (executed or not)
				out += !!cur->hits();
			}

			return out;
		}

		unsigned int getNrLines() const
		{
			return m_nrLines;
		}

		bool lineIsCode(unsigned int lineNr) const
		{
			if (lineNr >= m_lines.size())
				return false;

			return m_lines[lineNr] != NULL;
		}

	private:
		std::vector<Line *> m_lines;
		unsigned int m_nrLines;
	};

	typedef std::unordered_map<std::string, File> FileMap_t;
	typedef std::unordered_map<unsigned long, Line *> AddrToLineMap_t;
	typedef std::unordered_map<unsigned long, unsigned long> AddrToHitsMap_t;
	typedef std::vector<IReporter::IListener *> ListenerList_t;

	FileMap_t m_files;
	AddrToLineMap_t m_addrToLine;
	AddrToHitsMap_t m_pendingHits;
	ListenerList_t m_listeners;

	IFileParser &m_fileParser;
	ICollector &m_collector;
	IFilter &m_filter;
	enum IFileParser::PossibleHits m_maxPossibleHits;
};

IReporter &IReporter::create(IFileParser &parser, ICollector &collector, IFilter &filter)
{
	return *new Reporter(parser, collector, filter);
}
