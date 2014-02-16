#include <engine.hh>
#include <utils.hh>

#include <list>

using namespace kcov;

class EngineFactory : public IEngineFactory
{
public:
	EngineFactory()
	{
	}

	void registerEngine(IEngine &engine)
	{
		m_engines.push_back(&engine);
	}

	IEngine *matchEngine(const std::string &file)
	{
		IEngine *best = NULL;
		size_t sz;

		uint8_t *data = (uint8_t *)read_file(&sz, "%s", file.c_str());

		for (const auto &it : m_engines) {
			if (!best)
				best = it;

			if (it->matchFile(data, sz) > best->matchFile(data, sz))
				best = it;
		}

		return best;
	}

private:
	typedef std::list<IEngine *> EngineList_t;

	EngineList_t m_engines;
};


static EngineFactory *g_instance;
IEngineFactory &IEngineFactory::getInstance()
{
	if (!g_instance)
		g_instance = new EngineFactory();

	return *g_instance;
}
