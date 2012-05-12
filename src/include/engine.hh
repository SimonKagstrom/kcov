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
		ev_syscall     =  2,
		ev_crash       =  3,
		ev_exit        =  4,
	};

	class IEngine
	{
	public:
		class Event
		{
		public:
			enum event_type type;

			int data; // Typically the breakpoint
			unsigned long addr;
		};

		static IEngine &getInstance();

		/**
		 * Set a breakpoint
		 *
		 * @param addr the address to set the breakpoint on
		 *
		 * @return the ID of the breakpoint, or -1 on failure
		 */
		virtual int setBreakpoint(unsigned long addr) = 0;

		virtual bool clearBreakpoint(int id) = 0;


		virtual void clearAllBreakpoints() = 0;


		/**
		 * For a new process and attach to it with ptrace
		 *
		 * @return 0 when in the child, -1 on error or the pid otherwise
		 */
		virtual int start(const char *executable) = 0;

		/**
		 * Continue execution until next breakpoint.
		 *
		 * @return the event the execution stopped at.
		 */
		virtual const Event continueExecution() = 0;

		virtual std::string eventToName(Event ev) = 0;

		virtual void kill() = 0;
	};
}
