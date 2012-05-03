#pragma once

namespace kcov
{
	class IElf;
	class IReporter;
	class IOutputHandler;

	IWriter &createCoberturaWriter(IElf &elf, IReporter &reporter, IOutputHandler &output);
}
