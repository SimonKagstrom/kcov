#include <configuration.hh>
#include <engine.hh>
#include <reporter.hh>
#include <writer.hh>
#include <collector.hh>
#include <filter.hh>
#include <output-handler.hh>
#include <file-parser.hh>
#include <utils.hh>

#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "merge-parser.hh"
#include "html-writer.hh"
#include "cobertura-writer.hh"

using namespace kcov;

static IEngine *g_engine;
static IOutputHandler *g_output;
static ICollector *g_collector;
static IReporter *g_reporter;

static void do_cleanup()
{
	delete g_collector;
	delete g_output;
	delete g_engine;
	delete g_reporter;
}

static void ctrlc(int sig)
{
	// Forward the signal to the traced program
	g_engine->kill(sig);
}

static void daemonize(void)
{
	pid_t child;
	int res;

	IConfiguration &conf = IConfiguration::getInstance();
	std::string fifoName = conf.getOutDirectory() + conf.getBinaryName() + "/done.fifo";

	unlink(fifoName.c_str());
	res = mkfifo(fifoName.c_str(), 0600);

	panic_if(res < 0,
			"Can't create FIFO");

	child = fork();

	if (child < 0) {
		panic("Fork failed?\n");
	} else if (child == 0) {
		child = fork();

		if (child < 0) {
			panic("Fork failed?\n");
		} else if (child > 0) {
			// Second parent
			exit(0);
		}
	} else {
		// Parent
		FILE *fp;
		char buf[255];
		char *endp;
		char *p;
		unsigned long out;

		fp = fopen(fifoName.c_str(), "r");
		panic_if (!fp,
				"Can't open FIFO");
		p = fgets(buf, sizeof(buf), fp);
		panic_if (!p,
				"Can't read FIFO");


		out = strtoul(p, &endp, 10);
		if (p == endp)
			out = 0;

		exit(out);
	}
}

// Return the number of metadata directories in the kcov output path
unsigned int countMetadata()
{
	IConfiguration &conf = IConfiguration::getInstance();
	std::string base = conf.getOutDirectory();
	DIR *dir;
	struct dirent *de;

	dir = ::opendir(base.c_str());
	if (!dir)
		return 0;

	unsigned int out = 0;

	// Count metadata directories
	for (de = ::readdir(dir); de; de = ::readdir(dir)) {
		std::string cur = base + de->d_name + "/metadata";

		// ... except for the current coveree
		if (de->d_name == conf.getBinaryName())
			continue;

		DIR *metadataDir;
		struct dirent *de2;

		metadataDir = ::opendir(cur.c_str());
		if (!metadataDir)
			continue;

		// Count metadata files
		unsigned int datum = 0;
		for (de2 = ::readdir(metadataDir); de2; de2 = ::readdir(metadataDir)) {
			if (de2->d_name[0] == '.')
				continue;

			datum++;
		}
		out += !!datum;
		::closedir(metadataDir);
	}
	::closedir(dir);

	return out;
}


int main(int argc, const char *argv[])
{
	IConfiguration &conf = IConfiguration::getInstance();

	if (!conf.parse(argc, argv))
		return 1;

	std::string file = conf.getBinaryPath() + conf.getBinaryName();
	IFileParser *parser = IParserManager::getInstance().matchParser(file);
	if (!parser) {
		conf.printUsage();
		return 1;
	}
	parser->addFile(file);

	IEngine *engine = IEngineFactory::getInstance().matchEngine(file);
	if (!engine) {
		conf.printUsage();
		return 1;
	}

	IFilter &filter = IFilter::create();
	IFilter &dummyFilter = IFilter::createDummy();

	ICollector &collector = ICollector::create(*parser, *engine, filter);
	IReporter &reporter = IReporter::create(*parser, collector, filter);
	IOutputHandler &output = IOutputHandler::create(*parser, reporter);

	IConfiguration::RunMode_t runningMode = conf.getRunningMode();

	// Register writers
	if (runningMode != IConfiguration::MODE_COLLECT_ONLY) {
		const std::string &base = output.getBaseDirectory();
		const std::string &out = output.getOutDirectory();

		IWriter &htmlWriter = createHtmlWriter(*parser, reporter,
				base, out, conf.getBinaryName());
		IWriter &coberturaWriter = createCoberturaWriter(*parser, reporter,
				out + "/cobertura.xml");

		// The merge parser is both a parser, a writer and a collector (!)
		IMergeParser &mergeParser = createMergeParser(*parser,
				base, out, filter);
		IReporter &mergeReporter = IReporter::create(mergeParser, mergeParser, dummyFilter);
		IWriter &mergeHtmlWriter = createHtmlWriter(mergeParser, mergeReporter,
				base, base + "/kcov-merged", "[merged]", false);
		IWriter &mergeCoberturaWriter = createCoberturaWriter(mergeParser, mergeReporter,
				base + "kcov-merged/cobertura.xml");
		mkdir(fmt("%s/kcov-merged", base.c_str()).c_str(), 0755);

		collector.registerListener(mergeParser);

		output.registerWriter(mergeParser);
		// Only one covered binary? No need for merging writers then
		if (countMetadata() > 0) {
			output.registerWriter(mergeHtmlWriter);
			output.registerWriter(mergeCoberturaWriter);
		}
		output.registerWriter(htmlWriter);
		output.registerWriter(coberturaWriter);
	}

	g_engine = engine;
	g_output = &output;
	g_reporter = &reporter;
	g_collector = &collector;
	signal(SIGINT, ctrlc);
	signal(SIGTERM, ctrlc);

	if (conf.getExitFirstProcess())
		daemonize();

	output.start();

	int ret = 0;

	if (runningMode != IConfiguration::MODE_REPORT_ONLY) {
		ret = collector.run(file);
	} else {
		parser->parse();
	}

	do_cleanup();

	return ret;
}
