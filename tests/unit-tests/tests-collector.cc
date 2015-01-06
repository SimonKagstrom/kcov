#include "test.hh"
#include "mocks/mock-engine.hh"

#include <file-parser.hh>
#include <collector.hh>
#include <filter.hh>
#include <utils.hh>

#include <string>

using namespace kcov;

class MockCollectorListener : public ICollector::IListener
{
public:
	virtual ~MockCollectorListener()
	{
	}

	MAKE_MOCK2(onAddressHit, void(unsigned long addr, unsigned long hits));
};

DISABLED_TEST(collector)
{
	MockEngine engine;
	MockCollectorListener listener;
	IFileParser *elf;
	char filename[1024];
	bool res;

	sprintf(filename, "%s/test-binary", crpcut::get_start_dir());
	elf = IParserManager::getInstance().matchParser(filename);
	ASSERT_TRUE(elf);

	ICollector &collector = ICollector::create(*elf, engine, IFilter::create());

	collector.registerListener(listener);

	REQUIRE_CALL(engine, registerBreakpoint(_))
		.TIMES(AT_LEAST(1))
		.RETURN(0)
		;

	res = elf->parse();
	ASSERT_TRUE(res);

	int v;

	REQUIRE_CALL(engine, start(_,_))
		.TIMES(1)
		.RETURN(true)
		;

	IEngine::Event evExit, evOnce;

	evExit.addr = 0;
	evExit.type = ev_exit;
	evExit.data = 1;

	evOnce.addr = 1;
	evOnce.type = ev_breakpoint;
	evOnce.data = 1;

	REQUIRE_CALL(listener, onAddressHit(1, _))
		.TIMES(1)
		;

	v = collector.run(filename);

	ASSERT_EQ(v, evExit.data);

	evOnce.type = ev_error;

	// Test error
	REQUIRE_CALL(engine, start(_,_))
		.TIMES(1)
		.RETURN(0)
		;

	REQUIRE_CALL(engine, continueExecution())
		.TIMES(1)
		.RETURN(true)
		;
	v = collector.run(filename);

	ASSERT_EQ(v, -1);
}
