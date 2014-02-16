#include "test.hh"

#include <file-parser.hh>
#include <utils.hh>

#include <string>
#include <string.h>

using namespace kcov;

class FunctionListener : public IFileParser::ILineListener
{
public:

	virtual void onLine(const char *file, unsigned int lineNr,
			unsigned long addr)
	{
		m_lineMap[constructString(file, lineNr)]++;
	}

	static std::string constructString(const char *file, int nr)
	{
		char *c_str = (char *)xmalloc(strlen(file) + 20);
		const char *name = strrchr(file, '/');

		ASSERT_TRUE(name);

		sprintf(c_str, "%s:%d", name, nr);

		std::string out(c_str);
		free(c_str);

		return out;
	}

	std::map<std::string, int> m_lineMap;
};

TEST(elf, DEADLINE_REALTIME_MS(30000))
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

	res = elf->parse();
	ASSERT_TRUE(res == true);

	std::string str = FunctionListener::constructString("/test-source.c", 8);
	ASSERT_TRUE(listener.m_lineMap[str] > 0);
}
