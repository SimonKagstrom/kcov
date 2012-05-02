#include "test.hh"

#include <elf.hh>
#include <collector.hh>
#include <configuration.hh>
#include <reporter.hh>
#include <output-handler.hh>
#include <writer.hh>
#include <utils.hh>

#include <chrono>
#include <thread>

#include "../../src/html-writer.hh"

using namespace kcov;

class MockReporter : public IReporter
{
public:
	MOCK_METHOD2(lineIsCode,
			bool(const char *file, unsigned int lineNr));

	MOCK_METHOD2(getLineExecutionCount,
			LineExecutionCount(const char *file, unsigned int lineNr));

	MOCK_METHOD0(getExecutionSummary,
			ExecutionSummary());

	MOCK_METHOD1(marshal, void *(size_t *szOut));

	MOCK_METHOD2(unMarshal, bool(void *data, size_t sz));

	void *mockMarshal(size_t *outSz)
	{
		void *out = malloc(32);

		*outSz = 32;

		return out;
	}
};

TEST(writer)
{
	IElf *elf;
	bool res;
	char filename[1024];
	MockReporter reporter;
	IReporter::LineExecutionCount def(0, 1);
	IReporter::LineExecutionCount partial(1, 2);
	IReporter::LineExecutionCount full(3, 3);
	IReporter::ExecutionSummary summary(17, 4);

	sprintf(filename, "%s/test-binary", crpcut::get_start_dir());
	elf = IElf::open(filename);
	ASSERT_TRUE(elf);

	const char *argv[] = {NULL, "/tmp/vobb", filename};
	IConfiguration &conf = IConfiguration::getInstance();
	res = conf.parse(3, argv);
	ASSERT_TRUE(res);

	EXPECT_CALL(reporter, lineIsCode(_,_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(true))
		;
	EXPECT_CALL(reporter, lineIsCode(_,7))
		.Times(Exactly(2)) // Both files
		.WillRepeatedly(Return(false))
		;

	EXPECT_CALL(reporter, getLineExecutionCount(_,_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(def))
		;
	EXPECT_CALL(reporter, getLineExecutionCount(_,8))
		.Times(Exactly(2))
		.WillRepeatedly(Return(partial))
		;
	EXPECT_CALL(reporter, getLineExecutionCount(_,11))
		.Times(Exactly(1))
		.WillOnce(Return(full))
		;
	EXPECT_CALL(reporter, getExecutionSummary())
		.Times(AtLeast(1))
		.WillRepeatedly(Return(summary))
		;

	IOutputHandler &output = IOutputHandler::create(reporter);
	IWriter &writer = createHtmlWriter(*elf, reporter, output);

	output.registerWriter(writer);

	res = elf->parse();
	ASSERT_TRUE(res == true);
	output.start();

	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	EXPECT_CALL(reporter, marshal(_))
		.Times(Exactly(2))
		.WillRepeatedly(Invoke(&reporter, &MockReporter::mockMarshal))
		;

	output.stop();


	EXPECT_CALL(reporter, unMarshal(_,_))
		.Times(Exactly(1))
		.WillOnce(Return(true))
		;
	output.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	output.stop();
}
