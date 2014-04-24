#pragma once

namespace kcov
{
	class IFileParser;
	class IReporter;

	class IWriter
	{
	public:
		virtual ~IWriter() {}

		virtual void onStartup() = 0;

		virtual void onStop() = 0;

		virtual void write() = 0;
	};
}
