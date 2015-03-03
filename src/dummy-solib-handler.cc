#include <solib-handler.hh>

using namespace kcov;

class DummySolibHandler : public ISolibHandler
{
public:
	virtual void startup()
	{
	}
};

ISolibHandler &kcov::createSolibHandler(IFileParser &parser, ICollector &collector)
{
	return *new DummySolibHandler();
}
