#pragma once

namespace kcov
{
	class IElf;
	class IReporter;

	class IWriter
	{
	public:
		virtual void start() = 0;

		virtual void stop() = 0;

		static IWriter &create(IElf &elf, IReporter &reporter);
	};
}
