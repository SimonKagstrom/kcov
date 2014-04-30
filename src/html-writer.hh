#pragma once

namespace kcov
{
	class IFileParser;
	class IReporter;
	class IOutputHandler;

	IWriter &createHtmlWriter(IFileParser &elf, IReporter &reporter,
			const std::string &indexDirectory,
			const std::string &outDirectory,
			bool includeInTotals = true);
}
