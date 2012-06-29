#pragma once

namespace kcov
{
	class IElf;
	class IReporter;

	class IWriter
	{
	public:
		virtual void onStartup() = 0;

		virtual void onStop() = 0;

		virtual void write() = 0;


		virtual void stop() = 0;
	};
}
