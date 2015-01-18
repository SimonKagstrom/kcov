#include <engine.hh>
#include <utils.hh>
#include <configuration.hh>
#include <file-parser.hh>

using namespace kcov;

/*
 * From gcov-io.h:
 *
 *  int32:  byte3 byte2 byte1 byte0 | byte0 byte1 byte2 byte3
 *  int64:  int32:low int32:high
 *  string: int32:0 | int32:length char* char:0 padding
 *  padding: | char:0 | char:0 char:0 | char:0 char:0 char:0
 *  item: int32 | int64 | string
 *
 * File:
 *  file : int32:magic int32:version int32:stamp record*
 *
 * Record:
 *  record: header data
 *  header: int32:tag int32:length
 *  data: item*
 *
 * gcno file records:
 *  note: unit function-graph*
 *  unit: header int32:checksum string:source
 *  function-graph: announce_function basic_blocks {arcs | lines}*
 *  announce_function: header int32:ident
 *    int32:lineno_checksum int32:cfg_checksum
 *    string:name string:source int32:lineno
 *  basic_block: header int32:flags*
 *  arcs: header int32:block_no arc*
 *  arc:  int32:dest_block int32:flags
 *  lines: header int32:block_no line*
 *    int32:0 string:NULL
 *  line:  int32:line_no | int32:0 string:filename
 *
 * gcda file records:
 *   data: {unit summary:object summary:program* function-data*}*
 *   unit: header int32:checksum
 *   function-data:	announce_function present counts
 *   announce_function: header int32:ident
 *       int32:lineno_checksum int32:cfg_checksum
 *   present: header int32:present
 *   counts: header int64:count*
 *   summary: int32:checksum {count-summary}GCOV_COUNTERS_SUMMABLE
 *   count-summary:	int32:num int32:runs int64:sum
 *      int64:max int64:sum_max histogram
 *   histogram: {int32:bitvector}8 histogram-buckets*
 *     histogram-buckets: int32:num int64:min int64:sum
 */
#define GCOV_DATA_MAGIC ((uint32_t)0x67636461) /* "gcda" */
#define GCOV_NOTE_MAGIC ((uint32_t)0x67636e6f) /* "gcno" */

#define GCOV_TAG_FUNCTION	 ((uint32_t)0x01000000)
#define GCOV_TAG_FUNCTION_LENGTH (3)
#define GCOV_TAG_BLOCKS		 ((uint32_t)0x01410000)
#define GCOV_TAG_BLOCKS_LENGTH(NUM) (NUM)
#define GCOV_TAG_BLOCKS_NUM(LENGTH) (LENGTH)
#define GCOV_TAG_ARCS		 ((uint32_t)0x01430000)
#define GCOV_TAG_ARCS_LENGTH(NUM)  (1 + (NUM) * 2)
#define GCOV_TAG_ARCS_NUM(LENGTH)  (((LENGTH) - 1) / 2)
#define GCOV_TAG_LINES		 ((uint32_t)0x01450000)
#define GCOV_TAG_COUNTER_BASE 	 ((uint32_t)0x01a10000)
#define GCOV_TAG_COUNTER_LENGTH(NUM) ((NUM) * 2)
#define GCOV_TAG_COUNTER_NUM(LENGTH) ((LENGTH) / 2)
#define GCOV_TAG_OBJECT_SUMMARY  ((uint32_t)0xa1000000) /* Obsolete */
#define GCOV_TAG_PROGRAM_SUMMARY ((uint32_t)0xa3000000)
#define GCOV_TAG_SUMMARY_LENGTH(NUM)  \
        (1 + GCOV_COUNTERS_SUMMABLE * (10 + 3 * 2) + (NUM) * 5)
#define GCOV_TAG_AFDO_FILE_NAMES ((uint32_t)0xaa000000)
#define GCOV_TAG_AFDO_FUNCTION ((uint32_t)0xac000000)
#define GCOV_TAG_AFDO_WORKING_SET ((uint32_t)0xaf000000)

struct file_header
{
	int32_t magic;
	int32_t version;
	int32_t stamp;
};

struct header
{
	int32_t tag;
	int32_t length;
};

class GcovParser
{
public:
	GcovParser(const uint8_t *data, size_t dataSize) :
		m_data(data), m_dataSize(dataSize)
	{
	}

	virtual ~GcovParser()
	{
		free((void *)m_data);
	}

	bool parse()
	{
		// Not a gcov file?
		if (!verify())
			return false;

		const uint8_t *cur = m_data + sizeof(struct file_header);
		ssize_t left = (ssize_t)(m_dataSize - sizeof(struct file_header));

		// Iterate through the headers
		while (1) {
			const struct header *header = (struct header *)cur;

			if (!onRecord(header, cur + sizeof(*header)))
				return false;

			size_t curLen = sizeof(struct header) + header->length * 4;
			left -= curLen;

			if (left <= 0)
				break;

			cur += curLen;
		}

		return true;
	}

protected:
	virtual bool onRecord(const struct header *header, const uint8_t *data) = 0;

	bool verify()
	{
		const struct file_header *header = (const struct file_header *)m_data;

		if (header->magic != GCOV_DATA_MAGIC && header->magic != GCOV_NOTE_MAGIC)
			return false;

		return true;
	}

	const uint8_t *readString(const uint8_t *p, std::string &out)
	{
		int32_t length = *(const int32_t*)p;
		const char *c_str = (const char *)&p[4];

		out = std::string(c_str);

		return padPointer(p + length * 4 + 4); // Including the length field
	}

	// Convenience when using 32-bit pointers
	const int32_t *readString(const int32_t *p, std::string &out)
	{
		return (const int32_t *)readString((const uint8_t *)p, out);
	}


	const uint8_t *padPointer(const uint8_t *p)
	{
		unsigned long addr = (unsigned long)p;

		if ((addr & 3) != 0)
			p += 4 - (addr & 3);

		return p;
	}

	const uint8_t *m_data;
	size_t m_dataSize;
};

// Specialized parser for gcno files
class GcnoParser : public GcovParser
{
public:
	GcnoParser(const uint8_t *data, size_t dataSize) :
		GcovParser(data, dataSize)
	{
	}

protected:
	bool onRecord(const struct header *header, const uint8_t *data)
	{
		switch (header->tag)
		{
		case GCOV_TAG_FUNCTION:
			onAnnounceFunction(header, data);
			break;
		case GCOV_TAG_BLOCKS:
			onBlocks(header, data);
			break;
		case GCOV_TAG_LINES:
			onLines(header, data);
			break;
		case GCOV_TAG_ARCS:
			onArcs(header, data);
			break;
		default:
			break;
		}

		return true;
	}

private:
	void onAnnounceFunction(const struct header *header, const uint8_t *data)
	{
		const int32_t *p32 = (const int32_t *)data;
		const uint8_t *p8 = data;
		int32_t ident = p32[0];

		p8 = readString(p8 + 3 * 4, m_function);
		p8 = readString(p8, m_file);

		// The first line of this function
		p32 = (const int32_t *)p8;
		int32_t startLine = p32[0];

		printf("FN: %s, %s, 0x%08x, %d\n", m_function.c_str(), m_file.c_str(), ident, startLine);
	}

	void onBlocks(const struct header *header, const uint8_t *data)
	{
		const int32_t *p32 = (const int32_t *)data;

		printf("  BLOCKs %d\n", header->length);
		for (int32_t i = 0; i < header->length; i++)
			printf("   %2d: 0x%08x\n", i, p32[i]);
	}

	void onLines(const struct header *header, const uint8_t *data)
	{
		const int32_t *p32 = (const int32_t *)data;
		int32_t blockNo = p32[0];
		const int32_t *last = &p32[header->length];

		p32++; // Skip blockNo

		// Iterate through lines
		while (p32 < last) {
			int32_t line = *p32;

			// File name
			if (line == 0) {
				std::string name;

				// Setup current file name
				p32 = readString(p32 + 1, name);
				if (name != "")
					m_file = name;

				continue;
			}

			p32++;
			printf("  BB %2d: %s:%3d\n", blockNo, m_file.c_str(), line);
		}
	}

	void onArcs(const struct header *header, const uint8_t *data)
	{
		const int32_t *p32 = (const int32_t *)data;
		int32_t blockNo = p32[0];
		const int32_t *last = &p32[header->length];
		unsigned int arc = 0;

		p32++; // Skip blockNo

		// Iterate through lines
		while (p32 < last) {
			int32_t destBlock = p32[0];
			int32_t flags = p32[1];

			printf("  Arc%2d from %2d to %2d (%s %s %s)\n", arc, blockNo, destBlock, flags & 1 ? "OT" : "  ", flags & 2 ? "FK" : "  ", flags & 4 ? "TR" : "  ");
			p32 += 2;
			arc++;
		}
	}

	std::string m_file;
	std::string m_function;
};

class GcdaParser : public GcovParser
{
public:
	GcdaParser(const uint8_t *data, size_t dataSize) :
		GcovParser(data, dataSize)
	{
	}

protected:
	bool onRecord(const struct header *header, const uint8_t *data)
	{
		printf("DA: 0x%08x (%d)\n", header->tag, header->length);

		switch (header->tag)
		{
		case GCOV_TAG_FUNCTION:
			onAnnounceFunction(header, data);
			break;
		case GCOV_TAG_COUNTER_BASE:
			onCounterBase(header, data);
			break;
		default:
			break;
		}

		return true;
	}

	void onAnnounceFunction(const struct header *header, const uint8_t *data)
	{
		const int32_t *p32 = (const int32_t *)data;
		int32_t ident = p32[0];
		// FIXME! Handle checksums after this

		printf(" FN da ident %d\n", ident);
	}

	void onCounterBase(const struct header *header, const uint8_t *data)
	{
		const int64_t *p64 = (const int64_t *)data;
		int32_t count = header->length / 2; // 64-bit values

		for (int32_t i = 0; i <= count; i++)
			printf(" Cntr: %lld\n", (long long)p64[i]);
	}
};

class GcovEngine : public IEngine
{
public:
	GcovEngine()
	{
		IEngineFactory::getInstance().registerEngine(*this);
	}

	int registerBreakpoint(unsigned long addr)
	{
		return 0;
	}

	bool start(IEventListener &listener, const std::string &executable)
	{
		return true;
	}


	void kill(int sig)
	{
	}

	bool continueExecution()
	{
		return true;
	}

	unsigned int matchFile(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		size_t sz;
		void *d = read_file(&sz, "%s", "/home/ska/projects/build/kcov/build-tests/CMakeFiles/main-tests-gcov.dir/subdir2/file2.c.gcno");
		GcnoParser gcno((uint8_t *)d, sz);
		void *d2 = read_file(&sz, "%s", "/home/ska/projects/build/kcov/build-tests/CMakeFiles/main-tests-gcov.dir/subdir2/file2.c.gcda");
		GcdaParser gcda((uint8_t *)d2, sz);

		gcno.parse();
		gcda.parse();

		return match_none;
	}

private:
	class Ctor
	{
	public:
		Ctor()
		{
			m_engine = new GcovEngine();
		}

		~Ctor()
		{
			delete m_engine;
		}

		GcovEngine *m_engine;
	};
	static GcovEngine::Ctor g_gcovEngine;
};

GcovEngine::Ctor GcovEngine::g_gcovEngine;
