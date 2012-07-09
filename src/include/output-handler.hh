#pragma once

#include <string>

namespace kcov
{
	class IWriter;
	class IReporter;

	class IOutputHandler
	{
	public:
		virtual ~IOutputHandler() {}

		virtual void registerWriter(IWriter &writer) = 0;

		virtual void start() = 0;

		virtual void stop() = 0;


		virtual std::string getBaseDirectory() = 0;

		virtual std::string getOutDirectory() = 0;

		static IOutputHandler &create(IReporter &reporter);

		static IOutputHandler &getInstance();
	};
}
