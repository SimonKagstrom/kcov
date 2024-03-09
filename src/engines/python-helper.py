#!/usr/bin/env python3
# encoding: utf-8

# Based on http://pymotw.com/2/sys/tracing.html, "Tracing a program as it runs"
# and http://hg.python.org/cpython/file/2.7/Lib/trace.py

import sys
import os
import struct

fifo_file = None

if sys.version_info >= (3, 0):
    # In Py 3.x, they're in builtins
    BUILTINS = sys.modules["builtins"]

    def report_trace_real(file, line):
        size = len(file) + 1 + 8 + 4 + 4
        data = struct.pack(
            ">QLL%dsb" % len(file),
            0x6D6574616C6C6775,
            size,
            int(line),
            bytes(file, "utf-8"),
            0,
        )

        fifo_file.write(data)
else:
    # In Py 2.x, the builtins were in __builtin__
    BUILTINS = sys.modules["__builtin__"]

    def report_trace_real(file, line):
        size = len(file) + 1 + 8 + 4 + 4
        data = struct.pack(">QLL%dsb" % len(file), 0x6D6574616C6C6775, size, int(line), file, 0)

        fifo_file.write(data)


if sys.version_info >= (3, 5):
    # The imp module has been deprecated since version 3.4 and removed in
    # version 3.12.
    import types

    def new_module(name):
        return types.ModuleType(name)
else:
    import imp

    def new_module(name):
        return imp.new_module(name)


def report_trace(file, line):
    try:
        report_trace_real(file, line)
        fifo_file.flush()
    except:
        # Ignore errors
        pass


def trace_lines(frame, event, arg):
    if event != "line":
        return
    co = frame.f_code
    func_name = co.co_name
    line_no = frame.f_lineno
    filename = co.co_filename
    report_trace(filename, line_no)


def trace_calls(frame, event, arg):
    if event != "call":
        return
    co = frame.f_code
    func_name = co.co_name
    line_no = frame.f_lineno
    filename = co.co_filename
    report_trace(filename, line_no)
    return trace_lines


def runctx(cmd, globals):
    sys.settrace(trace_calls)
    try:
        exec(cmd, globals)
    finally:
        sys.settrace(None)


if __name__ == "__main__":
    prog_argv = sys.argv[1:]

    sys.argv = prog_argv
    progname = prog_argv[0]
    sys.path[0] = os.path.split(progname)[0]

    fifo_path = os.getenv("KCOV_PYTHON_PIPE_PATH")
    if fifo_path is None:
        sys.stderr.write("the KCOV_PYTHON_PIPE_PATH environment variable is not set")
        sys.exit(127)
    try:
        fifo_file = open(fifo_path, "wb")
    except:
        sys.stderr.write("Can't open fifo file")
        sys.exit(127)

    main_mod = new_module("__main__")
    old_main_mod = sys.modules["__main__"]
    sys.modules["__main__"] = main_mod
    main_mod.__file__ = progname
    main_mod.__package__ = None
    main_mod.__builtins__ = BUILTINS

    try:
        with open(progname) as fp:
            # try to emulate __main__ namespace as much as possible

            code = compile(fp.read(), progname, "exec")

            runctx(code, main_mod.__dict__)
    except IOError:
        sys.stderr.write("Cannot run file %r" % (sys.argv[0]))
        sys.exit(127)
    finally:
        sys.modules["__main__"] = old_main_mod
