#pragma once

#include <string>

namespace kcov
{
	class IFileParser;

	class IMergeParser :
		public IFileParser,
		public ICollector::IListener,
		public IWriter,
		public ICollector
	{
	};

	IMergeParser &createMergeParser(IFileParser &localParser,
			const std::string &baseDirectory,
			const std::string &outputDirectory,
			IFilter &filter);
}
