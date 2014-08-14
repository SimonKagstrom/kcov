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
	MOCK_METHOD2(onAddress, void(unsigned long addr, unsigned long hits));
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

	EXPECT_CALL(engine, registerBreakpoint(_))
		.Times(AtLeast(1))
		;

	res = elf->parse();
	ASSERT_TRUE(res);

	int v;

	EXPECT_CALL(engine, start(_,_))
		.Times(Exactly(1))
		.WillRepeatedly(Return(true))
		;

	IEngine::Event evExit, evOnce;

	evExit.addr = 0;
	evExit.type = ev_exit;
	evExit.data = 1;

	evOnce.addr = 1;
	evOnce.type = ev_breakpoint;
	evOnce.data = 1;

	EXPECT_CALL(listener, onAddress(1, _))
		.Times(Exactly(1))
		;

	EXPECT_CALL(engine, clearBreakpoint(_))
		.Times(Exactly(1))
		;

	v = collector.run(filename);

	ASSERT_TRUE(v == evExit.data);

	evOnce.type = ev_error;

	// Test error
	EXPECT_CALL(engine, start(_,_))
		.Times(Exactly(1))
		.WillRepeatedly(Return(0))
		;

	EXPECT_CALL(engine, continueExecution())
		.Times(Exactly(1))
		;
	v = collector.run(filename);

	ASSERT_TRUE(v == -1);
}
