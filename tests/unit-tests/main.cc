#include "test.hh"
#include "mocks/mock-engine.hh"

using namespace kcov;

IEngine &IEngine::getInstance()
{
	static MockEngine *instance;

	if (!instance)
		instance = new MockEngine();

	return *instance;
}


int main(int argc, const char *argv[])
{
	return crpcut::run(argc, argv);
}
