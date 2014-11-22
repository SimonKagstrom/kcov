#include "test.hh"

#include "mocks/mock-reporter.hh"
#include "../../src/merge-file-parser.cc"

#include <utils.hh>
#include <string>
#include <vector>

class MockParser : public IFileParser
{
public:
	MOCK_METHOD2(addFile, bool(const std::string &filename, struct phdr_data_entry *phdr_data));
	MOCK_METHOD1(registerLineListener, void(ILineListener &listener));
	MOCK_METHOD1(registerFileListener, void(IFileListener &listener));
	MOCK_METHOD0(parse, bool());
	MOCK_METHOD0(getChecksum, uint64_t());
	MOCK_METHOD0(getParserType, std::string());
	MOCK_METHOD1(setupParser, void(IFilter *));
	MOCK_METHOD3(matchParser, unsigned int(const std::string &filename, uint8_t *mock_data, size_t dataSize));
};

static bool mocked_file_exists(const std::string &path)
{
	return true;
}


static std::vector<uint8_t> mock_data;
static void *mocked_read_file(size_t *out_size, const char *path)
{
	*out_size = mock_data.size();

	void *p = xmalloc(*out_size);
	memcpy(p, mock_data.data(), *out_size);

	return p;
}

static std::unordered_map<std::string, std::string> path_to_data;
static int mocked_write_file(const void *data, size_t size, const char *path)
{
	path_to_data[path] = std::string((const char *)data);

	return 0;
}

static uint64_t mocked_ts;
static uint64_t mocked_get_timestamp(const std::string &filename)
{
	return mocked_ts;
}

class LineListenerFixture : public IFileParser::ILineListener
{
public:
	virtual ~LineListenerFixture()
	{
	}

	void onLine(const std::string &file, unsigned int lineNr,
			unsigned long addr)
	{
		m_lineToAddr[lineNr] = addr;
	}

protected:
	std::unordered_map<unsigned int, unsigned long> m_lineToAddr;
};

class AddressListenerFixture : public IReporter::IListener, public ICollector::IListener
{
public:
	virtual ~AddressListenerFixture()
	{
	}

	void onAddress(unsigned long addr, unsigned long hits)
	{
		m_addrToHits[addr] = hits;
	}

	void onAddressHit(unsigned long addr, unsigned long hits)
	{
		m_breakpointToHits[addr] = hits;
	}

protected:
	std::unordered_map<unsigned int, unsigned long> m_addrToHits;
	std::unordered_map<unsigned int, unsigned long> m_breakpointToHits;
};


TESTSUITE(merge_parser)
{
	TEST(marshal)
	{
		MockParser mockParser;
		MockReporter reporter;
		auto &filter = IFilter::create();

		EXPECT_CALL(mockParser, registerFileListener(_))
			.Times(Exactly(2))
		;
		EXPECT_CALL(mockParser, registerLineListener(_))
			.Times(Exactly(2))
		;

		mock_data = {'a', '\n', 'b', '\n', '\0'};
		mocked_ts = 1;

		MergeParser parser(mockParser, reporter, "/tmp", "/tmp/kalle", filter);

		mock_file_exists(mocked_file_exists);
		mock_read_file(mocked_read_file);
		mock_get_file_timestamp(mocked_get_timestamp);

		parser.onLine("a", 1, 2);
		parser.onLine("a", 1, 3); // Two addresses on the same line
		parser.onLine("a", 2, 3);

		parser.onLine("c", 2, 5);

		const struct file_data *p;

		// Does not exist
		p = parser.marshalFile("b");
		ASSERT_TRUE(!p);

		// But this one does
		p = parser.marshalFile("a");
		ASSERT_TRUE(p);

		ASSERT_TRUE(be_to_host<uint32_t>(p->magic) == MERGE_MAGIC);
		ASSERT_TRUE(be_to_host<uint32_t>(p->version) == MERGE_VERSION);
		ASSERT_TRUE(be_to_host<uint32_t>(p->n_entries) == 2);

		char *name = ((char *)p) + be_to_host<uint32_t>(p->file_name_offset);
		ASSERT_TRUE(std::string(name) == "a");
		ASSERT_TRUE(name + strlen(name) + 1 - (char *)p == be_to_host<uint32_t>(p->size));

		ASSERT_TRUE(be_to_host<uint32_t>(p->entries[0].line) == 1);
		ASSERT_TRUE(be_to_host<uint32_t>(p->entries[0].n_addresses) == 2);

		uint64_t *addressTable = (uint64_t *)((char *)p + be_to_host<uint32_t>(p->address_table_offset));
		uint32_t start = be_to_host<uint32_t>(p->entries[0].address_start);

		ASSERT_TRUE(be_to_host<uint64_t>(addressTable[start]) == parser.hashAddress("a", 1, 2));
		ASSERT_TRUE(be_to_host<uint64_t>(addressTable[start + 1]) == parser.hashAddress("a", 1, 3) + 1);

		// No output
		MergeParser parser2(mockParser, reporter, "/tmp", "/tmp/kalle", filter);

		parser2.onLine("c", 4, 1);
		// New timestamp for the "a" file
		mocked_ts = 2;
		parser2.onLine("a", 1, 2);

		const struct file_data *p2;

		// Test that the checksum changes on new TS
		p2 = parser2.marshalFile("a");
		ASSERT_TRUE(p2);

		ASSERT_TRUE(p->checksum == p2->checksum);
		ASSERT_TRUE(p->timestamp != p2->timestamp);

		// ... but is the same with the old TS
		p = parser.marshalFile("c");
		p2 = parser2.marshalFile("c");
		ASSERT_TRUE(p);
		ASSERT_TRUE(p2);

		// Should be the same
		ASSERT_TRUE(p->checksum == p2->checksum);
		ASSERT_TRUE(p->timestamp == p2->timestamp);

		// Same timestamp, different data
		parser.onLine("d", 9, 1);
		mock_data = {'a', '\n', 'b', 'c', '\n', '\0'};
		parser2.onLine("d", 9, 1);

		p = parser.marshalFile("d");
		p2 = parser2.marshalFile("d");
		ASSERT_TRUE(p);
		ASSERT_TRUE(p2);

		ASSERT_TRUE(p->timestamp == p2->timestamp);
		ASSERT_TRUE(p->checksum != p2->checksum);
	}

	TEST(output)
	{
		auto &filter = IFilter::create();
		MockParser mockParser;
		MockReporter reporter;

		EXPECT_CALL(mockParser, registerFileListener(_))
			.Times(Exactly(1))
		;
		EXPECT_CALL(mockParser, registerLineListener(_))
			.Times(Exactly(1))
		;

		mock_data = {'a', '\n', 'b', '\n', '\0'};
		mocked_ts = 1;

		MergeParser parser(mockParser, reporter, "/tmp", "/tmp/kalle", filter);

		mock_file_exists(mocked_file_exists);
		mock_read_file(mocked_read_file);
		mock_write_file(mocked_write_file);
		mock_get_file_timestamp(mocked_get_timestamp);

		parser.onLine("a", 1, 2);
		parser.onLine("b", 2, 3);

		parser.onStop();

		std::string p0 = parser.m_outputDirectory + "/metadata/" + fmt("%08x", crc32("a", 1));
		std::string p1 = parser.m_outputDirectory + "/metadata/" + fmt("%08x", crc32("b", 1));

		ASSERT_TRUE(path_to_data.find(p0) != path_to_data.end());
		ASSERT_TRUE(path_to_data.find(p1) != path_to_data.end());
	}

	TEST(input, LineListenerFixture, AddressListenerFixture)
	{
		auto &filter = IFilter::create();
		MockParser mockParser;
		MockReporter reporter;

		EXPECT_CALL(mockParser, registerFileListener(_))
			.Times(Exactly(2))
		;
		EXPECT_CALL(mockParser, registerLineListener(_))
			.Times(Exactly(2))
		;

		mock_data = {'a', '\n', 'b', '\n', '\0'};
		mocked_ts = 1;

		MergeParser parser(mockParser, reporter, "/tmp", "/tmp/kalle", filter);

		mock_file_exists(mocked_file_exists);
		mock_read_file(mocked_read_file);
		mock_write_file(mocked_write_file);
		mock_get_file_timestamp(mocked_get_timestamp);

		// Register the collector address listener
		parser.registerListener(*this);

		parser.onLine("a", 1, 2);
		parser.onLine("a", 3, 9);
		parser.onLine("a", 4, 72);
		parser.onLine("b", 2, 3);

		ASSERT_TRUE(m_breakpointToHits[parser.hashAddress("b", 2, 3)] == 0);
		parser.onAddress(3, 2);
		ASSERT_TRUE(m_breakpointToHits[parser.hashAddress("b", 2, 3)] == 2);

		// Register afterwards to get everything on parse below...
		parser.registerLineListener(*this);

		// Non-hash file
		parser.parseOne("/tmp/kalle/df/", "some-other-file");

		// Non-existing
		parser.parseOne("/tmp/kalle/df/", "12345678");

		// Existing (but fake, anyway)
		parser.parseOne("/tmp/kalle/df/",
				fmt("0x%08x", parser.m_files["a"]->m_checksum));

		const struct file_data *p;
		p = parser.marshalFile("a");
		ASSERT_TRUE(p);

		MergeParser parser2(mockParser, reporter, "/tmp", "/tmp/kalle", filter);
		parser2.registerLineListener(*this);
		parser2.registerListener(*this);

		// Valid file and data
		ASSERT_TRUE(m_lineToAddr.size() == 0);
		ASSERT_TRUE(m_breakpointToHits[parser.hashAddress("a", 4, 72)] == 0);

		// See to it that we "know of" file "a"
		parser2.onLine("a", 4, 72);

		// Mark all entries as executed in the table (should yield hits below)
		uint64_t *table = (uint64_t*)((const char *)p + be_to_host<uint32_t>(p->address_table_offset));

		for (unsigned int i = 0; i < 4; i++)
			table[i] = to_be<uint64_t>((be_to_host<uint64_t>(table[i]) | (1ULL << 63)));

		mock_data.clear();

		uint8_t *b = (uint8_t *)p;
		for (unsigned int i = 0; i < be_to_host<uint32_t>(p->size); i++)
			mock_data.push_back(b[i]);


		parser2.parseOne("/tmp/kalle/df/",
				fmt("0x%08x", parser.m_files["a"]->m_checksum));
		ASSERT_TRUE(m_lineToAddr.size() == 3);
		ASSERT_TRUE(m_lineToAddr[1] == parser.hashAddress("a", 1, 2));
		ASSERT_TRUE(m_breakpointToHits[parser.hashAddress("a", 4, 72)] == 1);

		free((void *)p);
	}
}
