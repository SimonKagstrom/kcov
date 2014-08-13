#include <engine.hh>
#include <utils.hh>

#include <vector>

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

	IEngine *matchEngine(const std::string &fileName)
	{
		static IEngine *best = NULL;
		size_t sz;

		// Cache best engine
		if (best)
			return best;

		uint8_t *data = (uint8_t *)read_file(&sz, "%s", fileName.c_str());

		for (EngineList_t::const_iterator it = m_engines.begin();
				it != m_engines.end();
				++it) {
			if (!best)
				best = *it;

			if ((*it)->matchFile(fileName, data, sz) > best->matchFile(fileName, data, sz))
				best = *it;
		}
		free((void *)data);

		return best;
	}

private:
	typedef std::vector<IEngine *> EngineList_t;

	EngineList_t m_engines;
};


static EngineFactory *g_instance;
IEngineFactory &IEngineFactory::getInstance()
{
	if (!g_instance)
		g_instance = new EngineFactory();

	return *g_instance;
}
