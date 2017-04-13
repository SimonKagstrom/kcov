#include "test.hh"

#include <configuration.hh>
#include <utils.hh>

#include "../../src/configuration.cc"

static bool runParse(std::string args)
{
	char *p = strdup(args.c_str());
	char *cur;
	std::list<std::string> argList;

	argList.push_back(std::string(crpcut::get_start_dir()) + "/test-binary");

	cur = strtok(p, " ");
	while (cur) {
		argList.push_back(std::string(cur));
		cur = strtok(NULL, " ");
	}
	free((void *)p);

	const char **argv = (const char **)malloc(argList.size() * sizeof(const char*));

	int i = 0;
	for (std::list<std::string>::iterator it = argList.begin();
			it != argList.end();
			it++) {
		// Yes, these are memory leaks.
		argv[i] = strdup((*it).c_str());
		i++;
	}

	return IConfiguration::getInstance().parse(argList.size(), argv);
}

TESTSUITE(configuration)
{
	DISABLED_TEST(basic)
	{
		std::string filename = std::string(crpcut::get_start_dir()) + "/test-binary";

		Configuration *conf = &(Configuration &)Configuration::getInstance();
		ASSERT_TRUE(conf);


		bool res = runParse(fmt("/tmp/vobb %s tjena", filename.c_str()));
		ASSERT_TRUE(res);

		const char **a = conf->getArgv();
		ASSERT_TRUE(filename == a[0]);
		ASSERT_TRUE(std::string("tjena") == a[1]);

		ASSERT_TRUE(conf->keyAsString("binary-name") == "test-binary");
		ASSERT_TRUE(conf->keyAsString("binary-path") == std::string(crpcut::get_start_dir()) + "/");

		res = runParse("-h");
		ASSERT_FALSE(res);

		res = runParse("-s vbb");
		ASSERT_FALSE(res);

		ASSERT_TRUE(conf->keyAsInt("low-limit") == 25);
		ASSERT_TRUE(conf->keyAsInt("high-limit")== 75U);

		res = runParse(fmt("-l 30,60 /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(res);

		ASSERT_TRUE(conf->keyAsInt("low-limit")== 30);
		ASSERT_TRUE(conf->keyAsInt("high-limit") == 60);

		res = runParse(fmt("-l 20 /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(!res);

		res = runParse(fmt("-l 40,50,90 /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(!res);

		res = runParse(fmt("-l 35,hej /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(!res);

		res = runParse(fmt("-l yo,89 /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(!res);

		res = runParse(fmt("/tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(res);

		res = runParse(fmt("/tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(res);

		res = runParse(fmt("--path-strip-level=ejNummer /tmp/vobb, %s", filename.c_str()));
		ASSERT_FALSE(res);

		res = runParse(fmt("--path-strip-level=3 /tmp/vobb, %s", filename.c_str()));
		ASSERT_TRUE(res);

		res = runParse(fmt("--include-path=/a,/b/c --exclude-pattern=d/e/f /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(res);

		ASSERT_TRUE(conf->keyAsList("include-path").size() == 2U);
		ASSERT_TRUE(conf->keyAsList("include-path")[0] == "/a");
		ASSERT_TRUE(conf->keyAsList("include-path")[1] == "/b/c");

		ASSERT_TRUE(conf->keyAsList("exclude-pattern").size() == 1U);
		ASSERT_TRUE(conf->keyAsList("exclude-pattern")[0] == "d/e/f");
		
		res = runParse(fmt("--strip-path=path"));
		ASSERT_TRUE(res);
		
		ASSERT_TRUE(conf->keyAsString("strip-path") == "path");

		res = runParse(fmt("--include-path=~/a /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(res);

		ASSERT_TRUE(conf->keyAsList("include-path").size() == 1U);
		ASSERT_TRUE(conf->keyAsList("include-path")[0] == fmt("%s/a", get_home()));

		ASSERT_TRUE(conf->keyAsInt("attach-pid") == 0U);
		res = runParse(fmt("-p ejNummer /tmp/vobb %s", filename.c_str()));
		ASSERT_FALSE(res);

		res = runParse(fmt("-p 10 /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(res);

		res = runParse(fmt("--pid=100 /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(res);
		ASSERT_TRUE(conf->keyAsInt("attach-pid") == 100U);

		ASSERT_TRUE(g_kcov_debug_mask == STATUS_MSG);
		res = runParse(fmt("--debug=7 /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(res);
		ASSERT_TRUE(g_kcov_debug_mask == 7);

		res = runParse(fmt("--exclude-pattern=a /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(res);

		auto ep = conf->keyAsList("exclude-pattern");
		ASSERT_TRUE(ep.size() == 1);
		ASSERT_TRUE(ep[0] == "a");

		res = runParse(fmt("--include-pattern=a,b,c /tmp/vobb %s", filename.c_str()));
		ASSERT_TRUE(res);
		ep = conf->keyAsList("include-pattern");
		ASSERT_TRUE(ep.size() == 3);
		ASSERT_TRUE(ep[0] == "a");
		ASSERT_TRUE(ep[1] == "b");
		ASSERT_TRUE(ep[2] == "c");
	}
}
