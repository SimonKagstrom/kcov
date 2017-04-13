#include "test.hh"

#include <file-parser.hh>
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

#include "../../src/writers/html-writer.hh"
#include "../../src/writers/cobertura-writer.hh"

#include "mocks/mock-collector.hh"
#include "mocks/mock-reporter.hh"

using namespace kcov;


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

DISABLED_TEST(writer, DEADLINE_REALTIME_MS(20000))
{
	IFileParser *elf;
	bool res;
	char filename[1024];
	MockReporter reporter;
	IReporter::LineExecutionCount def(0, 1, 0);
	IReporter::LineExecutionCount partial(1, 2, 0);
	IReporter::LineExecutionCount full(3, 3, 0);
	IReporter::ExecutionSummary summary(17, 4);

	std::string outDir = (std::string(crpcut::get_start_dir()) + "/kcov-writer");
	system(fmt("rm -rf %s", outDir.c_str()).c_str());

	sprintf(filename, "%s/test-binary", crpcut::get_start_dir());
	elf = IParserManager::getInstance().matchParser(filename);
	ASSERT_TRUE(elf);

	const char *argv[] = {NULL, outDir.c_str(), filename};
	IConfiguration &conf = IConfiguration::getInstance();
	res = conf.parse(3, argv);
	ASSERT_TRUE(res);

	REQUIRE_CALL(reporter, lineIsCode(_,_))
		.TIMES(AT_LEAST(1))
		.RETURN((true))
		;
	REQUIRE_CALL(reporter, fileIsIncluded(_))
		.TIMES(AT_LEAST(1))
		.RETURN((true))
		;
	REQUIRE_CALL(reporter, lineIsCode(_,7))
		.TIMES(AT_LEAST(4)) // Both files
		.RETURN((false))
		;

	REQUIRE_CALL(reporter, getLineExecutionCount(_,_))
		.TIMES(AT_LEAST(1))
		.RETURN((def))
		;
	REQUIRE_CALL(reporter, getLineExecutionCount(_,8))
		.TIMES(AT_LEAST(3))
		.RETURN((partial))
		;
	REQUIRE_CALL(reporter, getLineExecutionCount(_,11))
		.TIMES(AT_LEAST(2))
		.RETURN((full))
		;
	REQUIRE_CALL(reporter, getExecutionSummary())
		.TIMES(AT_LEAST(3))
		.RETURN((summary))
		;

	MockCollector collector;

	IOutputHandler &output = IOutputHandler::create(reporter, &collector);
	IWriter &writer = createHtmlWriter(*elf, reporter,
			output.getBaseDirectory(), output.getOutDirectory(), "kalle");
	IWriter &coberturaWriter = createCoberturaWriter(*elf, reporter,
			output.getOutDirectory() + "/cobertura.xml");

	output.registerWriter(writer);
	output.registerWriter(coberturaWriter);

	res = elf->addFile(filename);
	ASSERT_TRUE(res == true);
	res = elf->parse();
	ASSERT_TRUE(res == true);
	output.start();

	output.produce();

	REQUIRE_CALL(reporter, marshal(_))
		.TIMES(AT_LEAST(1))
		.LR_RETURN(reporter.mockMarshal(_1))
		;

	output.produce();

	ASSERT_TRUE(filePatternInDir((outDir + "/test-binary").c_str(), "test-source.c") >= 1);
	ASSERT_TRUE(file_exists((outDir + "/test-binary/cobertura.xml").c_str()));

	output.start();

	delete &output; // UGLY!
}


DISABLED_TEST(writerSameName, DEADLINE_REALTIME_MS(20000))
{
	IFileParser *elf;
	bool res;
	char filename[1024];
	MockReporter reporter;
	IReporter::LineExecutionCount def(0, 1, 0);
	IReporter::LineExecutionCount partial(1, 2, 1);
	IReporter::LineExecutionCount full(3, 3, 2);
	IReporter::ExecutionSummary summary(17, 4);

	std::string outDir = (std::string(crpcut::get_start_dir()) + "/kcov-writerSameName");
	system(fmt("rm -rf %s", outDir.c_str()).c_str());

	sprintf(filename, "%s/same-name-test", crpcut::get_start_dir());
	elf = IParserManager::getInstance().matchParser(filename);
	ASSERT_TRUE(elf);

	const char *argv[] = {NULL, outDir.c_str(), filename};
	IConfiguration &conf = IConfiguration::getInstance();
	res = conf.parse(3, argv);
	ASSERT_TRUE(res);

	REQUIRE_CALL(reporter, lineIsCode(_,_))
		.TIMES(AT_LEAST(1))
		.RETURN((true))
		;
	REQUIRE_CALL(reporter, fileIsIncluded(_))
		.TIMES(AT_LEAST(1))
		.RETURN((true))
		;

	REQUIRE_CALL(reporter, getLineExecutionCount(_,_))
		.TIMES(AT_LEAST(1))
		.RETURN((def))
		;
	REQUIRE_CALL(reporter, getExecutionSummary())
		.TIMES(AT_LEAST(2))
		.RETURN((summary))
		;

	MockCollector collector;
	IOutputHandler &output = IOutputHandler::create(reporter, &collector);
	IWriter &writer = createHtmlWriter(*elf, reporter,
			output.getBaseDirectory(), output.getOutDirectory(), "anka");

	output.registerWriter(writer);

	res = elf->addFile(filename);
	ASSERT_TRUE(res == true);
	res = elf->parse();
	ASSERT_TRUE(res == true);
	output.start();

	output.produce();

	REQUIRE_CALL(reporter, marshal(_))
		.TIMES(AT_LEAST(1))
		.LR_RETURN(reporter.mockMarshal(_1))
		;

	output.produce();

	int cnt = filePatternInDir((outDir + "/same-name-test").c_str(), "html");
	ASSERT_TRUE(cnt == 4); // index.html + 3 source files

	delete &output; // UGLY!
}
