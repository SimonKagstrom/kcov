#include "test.hh"
#include "mocks/mock-engine.hh"

using namespace kcov;

const char *kcov_version = "0";

int main(int argc, const char *argv[])
{
	return crpcut::run(argc, argv);
}
