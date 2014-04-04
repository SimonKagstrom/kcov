#include "test.hh"

#include <utils.hh>
#include <string>

#include "../../src/merge-file-parser.cc"

class MockParser : public IFileParser
{
public:
	MOCK_METHOD2(addFile, bool(const std::string &filename, struct phdr_data_entry *phdr_data));
	MOCK_METHOD1(registerLineListener, void(ILineListener &listener));
	MOCK_METHOD1(registerFileListener, void(IFileListener &listener));
	MOCK_METHOD0(parse, bool());
	MOCK_METHOD0(getChecksum, uint64_t());
	MOCK_METHOD3(matchParser, unsigned int(const std::string &filename, uint8_t *data, size_t dataSize));
};

TESTSUITE(merge_parser)
{
	TEST(marshal)
	{
		MockParser mockParser;

		EXPECT_CALL(mockParser, registerFileListener(_))
			.Times(Exactly(1))
		;
		EXPECT_CALL(mockParser, registerLineListener(_))
			.Times(Exactly(1))
		;

		MergeParser parser(mockParser);

		ASSERT_TRUE(parser.m_localEntries.empty());
		parser.onLine("a", 1, 2);
		ASSERT_TRUE(parser.m_localEntries.find(LineId("a", 1)) != parser.m_localEntries.end());

		const struct file_data *p;

		// Does not exist
		p = parser.marshalFile("b");
		ASSERT_TRUE(!p);

		// But this one does
		p = parser.marshalFile("a");
		ASSERT_TRUE(p);
	}
}
