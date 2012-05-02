#pragma once

namespace kcov
{
	class IElf;
	class IReporter;
	class IOutputHandler;

	IWriter &createHtmlWriter(IElf &elf, IReporter &reporter, IOutputHandler &output);
}
