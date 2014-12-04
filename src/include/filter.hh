#pragma once

#include <string>

namespace kcov
{
	/**
	 * Class for filtering output data.
	 *
	 * Used for the --include-path and --include-pattern command line options.
	 */
	class IFilter
	{
	public:
		class Handler
		{
		public:
			virtual bool includeFile(const std::string &file) = 0;
		};

		/**
		 * Run filters on @a path.
		 *
		 * @param path the path to check
		 *
		 * @return true if this path should be included in the output, false otherwise.
		 */
		virtual bool runFilters(const std::string &path) = 0;


		static IFilter &create();

		static IFilter &createDummy();


		virtual ~IFilter() {}
	};
}
