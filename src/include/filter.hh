#pragma once

#include <string>

namespace kcov
{
	class IFilter
	{
	public:
		class Handler
		{
		public:
			virtual bool includeFile(std::string file) = 0;
		};

		virtual bool runFilters(std::string file) = 0;


		static IFilter &getInstance();
	};
}
