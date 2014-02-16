#include <file-parser.hh>
#include <utils.hh>

#include <list>

using namespace kcov;

class ParserManager : public IParserManager
{
public:
	ParserManager()
	{
	}

	void registerParser(IFileParser &parser)
	{
		m_parsers.push_back(&parser);
	}

	IFileParser *matchParser(const std::string &fileName)
	{
		IFileParser *best = NULL;
		size_t sz;

		uint8_t *data = (uint8_t *)read_file(&sz, "%s", fileName.c_str());

		for (const auto &it : m_parsers) {
			unsigned int myVal = it->matchParser(fileName, data, sz);
			if (myVal == match_none)
				continue;

			if (!best)
				best = it;

			unsigned int bestVal = best->matchParser(fileName, data, sz);

			if (myVal > bestVal)
				best = it;
		}

		free((void *)data);

		return best;
	}

private:
	typedef std::list<IFileParser *> ParserList_t;

	ParserList_t m_parsers;
};


static ParserManager *g_instance;
IParserManager &IParserManager::getInstance()
{
	if (!g_instance)
		g_instance = new ParserManager();

	return *g_instance;
}
