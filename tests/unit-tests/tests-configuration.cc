#include "test.hh"

#include <configuration.hh>

#include "../../src/configuration.cc"

TEST(configuration)
{
	std::string filename = std::string(crpcut::get_start_dir()) + "/test-binary";

	const char *argv[] = {NULL, "/tmp/vobb", filename.c_str(), "tjena"};
	Configuration *conf = &(Configuration &)Configuration::getInstance();
	ASSERT_TRUE(conf);
	bool res = conf->parse(3, argv);
	ASSERT_TRUE(res);

	const char **a = conf->getArgv();
	ASSERT_TRUE(filename == a[0]);
	ASSERT_TRUE(std::string("tjena") == a[1]);

	ASSERT_TRUE(conf->getBinaryName() == "test-binary");
	ASSERT_TRUE(conf->getBinaryPath() == std::string(crpcut::get_start_dir()) + "/");

	const char *argvHelp[] = {NULL, "-h"};
	res = conf->parse(2, argvHelp);
	ASSERT_FALSE(res);

	const char *argvSortFail[] = {NULL, "-s vbb"};
	res = conf->parse(2, argvSortFail);
	ASSERT_FALSE(res);

	const char *limits = "30,60";
	const char *argv2[] = {NULL, "-l", limits, "/tmp/vobb", "--sort-type=p", filename.c_str()};

	ASSERT_TRUE(conf->getSortType() == IConfiguration::FILENAME);
	ASSERT_TRUE(conf->m_lowLimit == 25U);
	ASSERT_TRUE(conf->m_highLimit == 75U);

	res = conf->parse(6, argv2);
	ASSERT_TRUE(res);

	ASSERT_TRUE(conf->getSortType() == IConfiguration::PERCENTAGE);
	ASSERT_TRUE(conf->m_lowLimit == 30U);
	ASSERT_TRUE(conf->m_highLimit == 60U);

	limits = "20"; argv2[2] =  limits;
	res = conf->parse(5, argv2);
	ASSERT_TRUE(!res);

	limits = "40,50,90"; argv2[2] =  limits;
	res = conf->parse(5, argv2);
	ASSERT_TRUE(!res);

	limits = "35,hej"; argv2[2] =  limits;
	res = conf->parse(5, argv2);
	ASSERT_TRUE(!res);

	limits = "yo,89"; argv2[2] =  limits;
	res = conf->parse(5, argv2);
	ASSERT_TRUE(!res);

	const char *stripLevel[] = {NULL, "--path-strip-level=3", "/tmp/vobb", filename.c_str()};
	const char *stripLevelFail[] = {NULL, "--path-strip-level=ejNummer", "/tmp/vobb", filename.c_str()};

	res = conf->parse(4, stripLevelFail);
	ASSERT_FALSE(res);

	res = conf->parse(4, stripLevel);
	ASSERT_TRUE(res);

	const char *argvPattern[] = {NULL, "--include-path=/a,/b/c", "--exclude-pattern=d/e/f", "/tmp/vobb", filename.c_str()};

	res = conf->parse(5, argvPattern);
	ASSERT_TRUE(res);

	ASSERT_TRUE(conf->getOnlyIncludePath().size() == 2U);
	ASSERT_TRUE(conf->getOnlyIncludePath()[0] == "/a");
	ASSERT_TRUE(conf->getOnlyIncludePath()[1] == "/b/c");

	ASSERT_TRUE(conf->getExcludePattern().size() == 1U);
	ASSERT_TRUE(conf->getExcludePattern()[0] == "d/e/f");
}
