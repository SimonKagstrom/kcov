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
#include <sys/types.h>
#include <dirent.h>

#include "../../src/html-writer.hh"
#include "../../src/cobertura-writer.hh"

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

	MOCK_METHOD0(stop, void());

	void *mockMarshal(size_t *outSz)
	{
		void *out = malloc(32);

		*outSz = 32;

		return out;
	}
};

static int filePatternInDir(const char *name, const char *pattern)
{
	DIR *d = opendir(name);
	ASSERT_TRUE(d);

	struct dirent *de = readdir(d);
	int out = 0;

	while (de)
	{
		if (strstr(de->d_name, pattern))
			out++;
		de = readdir(d);
	}

	return out;
}

TEST(writer, DEADLINE_REALTIME_MS(20000))
{
	IElf *elf;
	bool res;
	char filename[1024];
	MockReporter reporter;
	IReporter::LineExecutionCount def(0, 1);
	IReporter::LineExecutionCount partial(1, 2);
	IReporter::LineExecutionCount full(3, 3);
	IReporter::ExecutionSummary summary(17, 4);

	std::string outDir = (std::string(crpcut::get_start_dir()) + "kcov-writer");

	sprintf(filename, "%s/test-binary", crpcut::get_start_dir());
	elf = IElf::open(filename);
	ASSERT_TRUE(elf);

	const char *argv[] = {NULL, outDir.c_str(), filename};
	IConfiguration &conf = IConfiguration::getInstance();
	res = conf.parse(3, argv);
	ASSERT_TRUE(res);

	EXPECT_CALL(reporter, lineIsCode(_,_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(true))
		;
	EXPECT_CALL(reporter, lineIsCode(_,7))
		.Times(AtLeast(4)) // Both files
		.WillRepeatedly(Return(false))
		;

	EXPECT_CALL(reporter, getLineExecutionCount(_,_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(def))
		;
	EXPECT_CALL(reporter, getLineExecutionCount(_,8))
		.Times(AtLeast(3))
		.WillRepeatedly(Return(partial))
		;
	EXPECT_CALL(reporter, getLineExecutionCount(_,11))
		.Times(AtLeast(2))
		.WillRepeatedly(Return(full))
		;
	EXPECT_CALL(reporter, getExecutionSummary())
		.Times(AtLeast(3))
		.WillRepeatedly(Return(summary))
		;

	IOutputHandler &output = IOutputHandler::create(reporter);
	IWriter &writer = createHtmlWriter(*elf, reporter, output);
	IWriter &coberturaWriter = createCoberturaWriter(*elf, reporter, output);

	output.registerWriter(writer);
	output.registerWriter(coberturaWriter);

	EXPECT_CALL(reporter, unMarshal(_,_))
		.Times(Exactly(1))
		.WillOnce(Return(true))
		;

	res = elf->parse();
	ASSERT_TRUE(res == true);
	output.start();

	std::this_thread::sleep_for(std::chrono::milliseconds(800));

	EXPECT_CALL(reporter, marshal(_))
		.Times(AtLeast(1))
		.WillRepeatedly(Invoke(&reporter, &MockReporter::mockMarshal))
		;

	output.stop();

	ASSERT_TRUE(filePatternInDir((outDir + "/test-binary").c_str(), "test-source.c") == 1);
	ASSERT_TRUE(file_exists((outDir + "/test-binary/cobertura.xml").c_str()));

	output.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	output.stop();
}


TEST(writerSameName, DEADLINE_REALTIME_MS(20000))
{
	IElf *elf;
	bool res;
	char filename[1024];
	MockReporter reporter;
	IReporter::LineExecutionCount def(0, 1);
	IReporter::LineExecutionCount partial(1, 2);
	IReporter::LineExecutionCount full(3, 3);
	IReporter::ExecutionSummary summary(17, 4);

	std::string outDir = (std::string(crpcut::get_start_dir()) + "kcov-writerSameName");

	sprintf(filename, "%s/same-name-test", crpcut::get_start_dir());
	elf = IElf::open(filename);
	ASSERT_TRUE(elf);

	const char *argv[] = {NULL, outDir.c_str(), filename};
	IConfiguration &conf = IConfiguration::getInstance();
	res = conf.parse(3, argv);
	ASSERT_TRUE(res);

	EXPECT_CALL(reporter, lineIsCode(_,_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(true))
		;

	EXPECT_CALL(reporter, getLineExecutionCount(_,_))
		.Times(AtLeast(1))
		.WillRepeatedly(Return(def))
		;
	EXPECT_CALL(reporter, getExecutionSummary())
		.Times(AtLeast(2))
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
		.Times(AtLeast(1))
		.WillRepeatedly(Invoke(&reporter, &MockReporter::mockMarshal))
		;

	output.stop();

	int cnt = filePatternInDir((outDir + "/same-name-test").c_str(), "html");
	ASSERT_TRUE(cnt == 4); // index.html + 3 source files
}
