#include <engine.hh>
#include <utils.hh>
#include <configuration.hh>
#include <file-parser.hh>
#include <gcov.hh>

#include <vector>
#include <unordered_map>

using namespace kcov;

class GcovEngine : public IEngine
{
public:
	GcovEngine()
	{
		IEngineFactory::getInstance().registerEngine(*this);
	}

	int registerBreakpoint(unsigned long addr)
	{
		return 0;
	}

	bool start(IEventListener &listener, const std::string &executable)
	{
		return true;
	}


	void kill(int sig)
	{
	}

	bool continueExecution()
	{
		return true;
	}

	unsigned int matchFile(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		size_t sz;
		void *d = read_file(&sz, "%s", "/home/ska/projects/build/kcov/build-tests/CMakeFiles/main-tests-gcov.dir/subdir2/file2.c.gcno");
		GcnoParser gcno((uint8_t *)d, sz);
		void *d2 = read_file(&sz, "%s", "/home/ska/projects/build/kcov/build-tests/CMakeFiles/main-tests-gcov.dir/subdir2/file2.c.gcda");
		GcdaParser gcda((uint8_t *)d2, sz);

		gcno.parse();
		gcda.parse();

		const GcnoParser::BasicBlockList_t &bbs = gcno.getBasicBlocks();
		const GcnoParser::ArcList_t &arcs = gcno.getArcs();

		for (GcnoParser::BasicBlockList_t::const_iterator it = bbs.begin();
				it != bbs.end();
				++it) {
			const GcnoParser::BasicBlockMapping &cur = *it;

			printf("  BB %2d: %s:%3d\n", cur.m_basicBlock, cur.m_file.c_str(), cur.m_line);
		}
		for (GcnoParser::ArcList_t::const_iterator it = arcs.begin();
				it != arcs.end();
				++it) {
			const GcnoParser::Arc &cur = *it;

			printf("  Arc from %2d to %2d\n", cur.m_srcBlock, cur.m_dstBlock);
		}

		return match_none;
	}

private:

	class Ctor
	{
	public:
		Ctor()
		{
			m_engine = new GcovEngine();
		}

		~Ctor()
		{
			delete m_engine;
		}

		GcovEngine *m_engine;
	};
	static GcovEngine::Ctor g_gcovEngine;
};

GcovEngine::Ctor GcovEngine::g_gcovEngine;
