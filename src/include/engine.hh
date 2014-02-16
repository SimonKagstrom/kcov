#pragma once

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

	enum match_type
	{
		match_none = 0,
		match_perfect = 0xffffffff,
	};

	class IEngine
	{
	public:
		class Event
		{
		public:
			Event() :
				type(ev_signal),
				data(0),
				addr(0)
			{
			}

			enum event_type type;

			int data; // Typically the breakpoint
			unsigned long addr;
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
		virtual bool start(const std::string &executable) = 0;

		/**
		 * Wait for an event from the children
		 *
		 * @return the event the execution stopped at.
		 */
		virtual const Event waitEvent() = 0;

		/**
		 * Continue execution with an event
		 */
		virtual void continueExecution(const Event ev) = 0;


		virtual bool childrenLeft() = 0;

		virtual std::string eventToName(Event ev) = 0;

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
