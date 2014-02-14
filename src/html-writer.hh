#pragma once

namespace kcov
{
	class IFileParser;
	class IReporter;
	class IOutputHandler;

	IWriter &createHtmlWriter(IFileParser &elf, IReporter &reporter, IOutputHandler &output);
}
