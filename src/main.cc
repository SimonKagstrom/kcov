#include <configuration.hh>
#include <engine.hh>
#include <reporter.hh>
#include <writer.hh>
#include <collector.hh>
#include <output-handler.hh>
#include <file-parser.hh>
#include <utils.hh>

#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "html-writer.hh"
#include "cobertura-writer.hh"

using namespace kcov;

static IOutputHandler *g_output;
static ICollector *g_collector;
static IReporter *g_reporter;

static void ctrlc(int sig)
{
	g_collector->stop();
	g_reporter->stop();
	g_output->stop();
	IEngine::getInstance().kill();
	exit(0);
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


int main(int argc, const char *argv[])
{
	IConfiguration &conf = IConfiguration::getInstance();

	if (!conf.parse(argc, argv))
		return 1;

	std::string file = conf.getBinaryPath() + conf.getBinaryName();
	IFileParser *elf = IFileParser::open(file.c_str());
	if (!elf) {
		conf.printUsage();
		return 1;
	}

	ICollector &collector = ICollector::create(elf);
	IReporter &reporter = IReporter::create(*elf, collector);
	IOutputHandler &output = IOutputHandler::create(reporter);

	IConfiguration::RunMode_t runningMode = conf.getRunningMode();

	// Register writers
	if (runningMode != IConfiguration::MODE_COLLECT_ONLY) {
		IWriter &htmlWriter = createHtmlWriter(*elf, reporter, output);
		IWriter &coberturaWriter = createCoberturaWriter(*elf, reporter, output);
		output.registerWriter(htmlWriter);
		output.registerWriter(coberturaWriter);
	}

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
		elf->parse();
	}

	output.stop();
	IEngine::getInstance().kill();

	return ret;
}
