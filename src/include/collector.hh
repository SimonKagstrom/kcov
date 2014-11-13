#pragma once

#include <string>

namespace kcov
{
	class IFileParser;
	class IEngine;
	class IFilter;

	class ICollector
	{
	public:
		class IListener
		{
		public:
			virtual void onAddressHit(unsigned long addr, unsigned long hits) = 0;
		};

		class IEventTickListener
		{
		public:
			virtual void onTick() = 0;
		};

		static ICollector &create(IFileParser &elf, IEngine &engine, IFilter &filter);

		virtual void registerListener(IListener &listener) = 0;

		/**
		 * Register a listener for events (called on each breakpoint)
		 */
		virtual void registerEventTickListener(IEventTickListener &listener) = 0;

		virtual int run(const std::string &filename) = 0;

		virtual ~ICollector() {};
	};
}
