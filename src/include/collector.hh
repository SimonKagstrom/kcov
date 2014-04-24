#pragma once

#include <string>

namespace kcov
{
	class IFileParser;
	class IEngine;

	class ICollector
	{
	public:
		class IListener
		{
		public:
			virtual void onAddress(unsigned long addr, unsigned long hits) = 0;
		};

		static ICollector &create(IFileParser &elf, IEngine &engine);

		virtual void registerListener(IListener &listener) = 0;


		virtual int run(const std::string &filename) = 0;

		virtual ~ICollector() {};
	};
}
