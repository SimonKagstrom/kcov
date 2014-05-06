#pragma once

namespace kcov
{
	class IFileParser;
	class IReporter;
	class IOutputHandler;

	IWriter &createCoberturaWriter(IFileParser &elf, IReporter &reporter,
			const std::string &outFile);
}
