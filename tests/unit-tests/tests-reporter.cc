#include "test.hh"

#include <file-parser.hh>
#include <collector.hh>
#include <reporter.hh>
#include <filter.hh>
#include <utils.hh>

#include <string>
#include <unordered_map>

#include "../../src/reporter.cc"
#include "mocks/mock-collector.hh"

using namespace kcov;

class ElfListener : public IFileParser::ILineListener
{
public:
	virtual ~ElfListener()
	{
	}

	void onLine(const std::string &file, unsigned int lineNr, uint64_t addr)
	{
		// Just store the lastest to have something
		m_lineToAddr[lineNr] = addr;
		if (m_file == "")
			m_file = file;
	}

	std::string m_file;
	std::unordered_map<unsigned int, unsigned long> m_lineToAddr;
};

DISABLED_TEST(reporter)
{
	ElfListener elfListener;
	IFileParser *elf;
	bool res;
	char filename[1024];

	sprintf(filename, "%s/test-binary", crpcut::get_start_dir());
	elf = IParserManager::getInstance().matchParser(filename);
	ASSERT_TRUE(elf);

	elf->registerLineListener(elfListener);

	MockCollector collector;

	REQUIRE_CALL(collector, registerListener(_))
		.TIMES(1)
		.LR_SIDE_EFFECT(collector.mockRegisterListener(_1))
		;


	Reporter &reporter = (Reporter &)IReporter::create(*elf, collector, IFilter::create());

	IReporter::ExecutionSummary summary =
			reporter.getExecutionSummary();
	ASSERT_TRUE(summary.m_lines == 0U);
	ASSERT_TRUE(summary.m_executedLines == 0U);

	res = elf->addFile(filename);
	ASSERT_TRUE(res);

	res = elf->parse();
	ASSERT_TRUE(res);

	summary = reporter.getExecutionSummary();
	ASSERT_TRUE(summary.m_lines == 17U); // Executable lines
	ASSERT_TRUE(summary.m_executedLines == 0U);


	// Test something which doesn't exist
	collector.m_listener->onAddressHit(100, 1);

	summary = reporter.getExecutionSummary();
	ASSERT_TRUE(summary.m_executedLines == 0U);

	// See test-source.c
	IReporter::LineExecutionCount lc =
			reporter.getLineExecutionCount(elfListener.m_file.c_str(), 19);
	ASSERT_TRUE(lc.m_hits == 0U);
	ASSERT_TRUE(lc.m_possibleHits == 1U);


	ASSERT_TRUE(elfListener.m_lineToAddr[8]);

	// and something which does
	collector.m_listener->onAddressHit(elfListener.m_lineToAddr[19], 1);

	lc = reporter.getLineExecutionCount(elfListener.m_file.c_str(), 19);
	ASSERT_TRUE(lc.m_hits == 1U);
	ASSERT_TRUE(lc.m_possibleHits == 1U);

	// Once again (should not happen except on marshalling - this should
	// not count up the number of hits)
	collector.m_listener->onAddressHit(elfListener.m_lineToAddr[19], 1);
	lc = reporter.getLineExecutionCount(elfListener.m_file.c_str(), 19);
	ASSERT_TRUE(lc.m_hits == 1U);

	summary = reporter.getExecutionSummary();
	ASSERT_TRUE(summary.m_executedLines == 1U);

	res = reporter.lineIsCode(elfListener.m_file.c_str(), 19);
	ASSERT_TRUE(res == true);

	res = reporter.lineIsCode(elfListener.m_file.c_str(), 13);
	ASSERT_TRUE(res == false);

	// Test marshal and unmarshal
	collector.m_listener->onAddressHit(elfListener.m_lineToAddr[16], 1);

	summary = reporter.getExecutionSummary();
	ASSERT_TRUE(summary.m_executedLines == 2U);

	size_t sz;
	void *data = reporter.marshal(&sz);
	ASSERT_TRUE(sz >= 0U);
	ASSERT_TRUE(data);

	// Happened after, not part of the marshalling
	collector.m_listener->onAddressHit(elfListener.m_lineToAddr[17], 1);

	summary = reporter.getExecutionSummary();
	ASSERT_TRUE(summary.m_executedLines == 3U);

	res = reporter.unMarshal(data, sz);
	ASSERT_TRUE(res);
	lc = reporter.getLineExecutionCount(elfListener.m_file.c_str(), 16);
	ASSERT_TRUE(lc.m_hits == 1U);
	ASSERT_TRUE(lc.m_possibleHits == 1U);

	struct marshalHeaderStruct *hdr = (struct marshalHeaderStruct *)data;
	hdr->checksum++;
	res = reporter.unMarshal(data, sz);
	ASSERT_FALSE(res);
	hdr->checksum--;

	hdr->db_version++;
	res = reporter.unMarshal(data, sz);
	ASSERT_FALSE(res);
	hdr->db_version--;

	hdr->magic++;
	res = reporter.unMarshal(data, sz);
	ASSERT_FALSE(res);

	free(data);
}
