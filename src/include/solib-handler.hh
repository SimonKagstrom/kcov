#pragma once

#include <engine.hh>
#include <collector.hh>

namespace kcov
{
	class ISolibHandler
	{
	public:
		virtual ~ISolibHandler()
		{
		}
	};

	ISolibHandler &createSolibHandler(IFileParser &parser, ICollector &collector);

	// Wait for the solib thread to finish parsing the solib data
	void blockUntilSolibDataRead();
}
