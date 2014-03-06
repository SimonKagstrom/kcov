#pragma once

#include <manager.hh>

#include <stdint.h>
#include <stdlib.h>

#include <string>

namespace kcov
{
	enum event_type
	{
		ev_error       = -1,
		ev_breakpoint  =  1,
		ev_signal      =  2,
		ev_exit        =  3,
		ev_exit_first_process = 4,
	};

	class IEngine
	{
	public:
		class Event
		{
		public:
			Event(enum event_type type = ev_signal, int data = 0, uint64_t address = 0) :
				type(type),
				data(data),
				addr(address)
			{
			}

			enum event_type type;

			int data; // Typically the breakpoint
			uint64_t addr;
		};

		class IEventListener
		{
		public:
			virtual void onEvent(const Event &ev) = 0;
		};


		virtual ~IEngine() {}

		/**
		 * Set a breakpoint
		 *
		 * @param addr the address to set the breakpoint on
		 *
		 * @return the ID of the breakpoint, or -1 on failure
		 */
		virtual int registerBreakpoint(unsigned long addr) = 0;

		virtual void setupAllBreakpoints() = 0;

		virtual bool clearBreakpoint(int id) = 0;


		/**
		 * For a new process and attach to it with ptrace
		 *
		 * @return true if OK, false otherwise
		 */
		virtual bool start(IEventListener &listener, const std::string &executable) = 0;

		/**
		 * Continue execution with an event
		 *
		 * @return true if the process should continue, false otherwise
		 */
		virtual bool continueExecution() = 0;


		virtual bool childrenLeft() = 0;

		virtual void kill() = 0;

		virtual unsigned int matchFile(const std::string &filename, uint8_t *data, size_t dataSize) = 0;
	};

	/**
	 * Factory class for getting one of multiple engines, which can
	 * match different file types.
	 */
	class IEngineFactory
	{
	public:
		virtual ~IEngineFactory()
		{
		}

		virtual void registerEngine(IEngine &engine) = 0;

		virtual IEngine *matchEngine(const std::string &file) = 0;

		static IEngineFactory &getInstance();
	};
}
