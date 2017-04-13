#include "test.hh"

#include <file-parser.hh>
#include <utils.hh>

#include <string>
#include <string.h>

#include <map>

using namespace kcov;

class FunctionListener : public IFileParser::ILineListener
{
public:

	virtual void onLine(const std::string &file, unsigned int lineNr,
			uint64_t addr)
	{
		m_lineMap[constructString(file, lineNr)]++;
	}

	static std::string constructString(const std::string &file, int nr)
	{
		const char *p = file.c_str();
		char *c_str = (char *)xmalloc(file.size() + 20);
		const char *name = strrchr(p, '/');

		ASSERT_TRUE(name);

		sprintf(c_str, "%s:%d", name, nr);

		std::string out(c_str);
		free(c_str);

		return out;
	}

	std::map<std::string, int> m_lineMap;
};

DISABLED_TEST(elf, DEADLINE_REALTIME_MS(30000))
{
	FunctionListener listener;
	char filename[1024];
	bool res;

	IFileParser *elf = IParserManager::getInstance().matchParser("not-found");
	ASSERT_TRUE(!elf);

	sprintf(filename, "%s/Makefile", crpcut::get_start_dir());
	elf = IParserManager::getInstance().matchParser(filename);
	ASSERT_TRUE(!elf);

	sprintf(filename, "%s/test-binary", crpcut::get_start_dir());
	elf = IParserManager::getInstance().matchParser(filename);
	ASSERT_TRUE(elf);

	elf->registerLineListener(listener);

	res = elf->addFile(filename);
	ASSERT_TRUE(res == true);
	res = elf->parse();
	ASSERT_TRUE(res == true);

	std::string str = FunctionListener::constructString("/test-source.c", 8);
	ASSERT_TRUE(listener.m_lineMap[str] > 0);
}
