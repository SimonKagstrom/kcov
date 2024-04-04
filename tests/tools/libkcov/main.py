"""A custom simplified implementation of the test runner.

This is necessary, since `unittest.main` does not support correctly how kcov
tests are imported.  It will also make it easy to add additional features.

Most of the test runner code has been copied from the `unittest.main` module.
"""

import argparse
import importlib
import os
import os.path
import platform
import random
import sys
import unittest
from collections import namedtuple
from fnmatch import fnmatchcase

# Copied from unittest.main.
_NO_TESTS_EXITCODE = 5


class TestLoader:
    """A simplified test loader.

    The implementation assumes that each test case runs only one test method:
    `runTest`.
    """

    def __init__(self, config, patterns):
        self.tests = []
        self.config = config
        self.patterns = patterns

    def add_tests_from_module(self, name):
        """Add all test cases from the named module."""

        module = importlib.import_module(name)
        for name in dir(module):
            obj = getattr(module, name)
            if (
                isinstance(obj, type)
                and issubclass(obj, unittest.TestCase)
                and obj not in (unittest.TestCase, unittest.FunctionTestCase)
                and hasattr(obj, "runTest")
            ):
                if not self.match_test(obj):
                    continue

                cfg = self.config
                test = obj(cfg.kcov, cfg.outbase, cfg.testbuild, cfg.sources)
                self.tests.append(test)

    def match_test(self, test_case_class):
        if not self.patterns:
            return True

        full_name = f"{test_case_class.__module__}.{test_case_class.__qualname__}"
        return any(fnmatchcase(full_name, pattern) for pattern in self.patterns)


Config = namedtuple("Config", ["kcov", "outbase", "testbuild", "sources"])


def fatal(msg):
    sys.stderr.write(f"error: {msg}\n")
    sys.stderr.flush()
    os._exit(1)


def normalized_not_empty(path):
    """Normalize path, also ensuring that it is not empty."""

    if len(path) == 0:
        raise ValueError("path must be not empty")

    path = os.path.normpath(path)
    return path


# Implementation copied from unittest.main.
def to_fnmatch(pattern):
    if "*" not in pattern:
        pattern = "*%s*" % pattern

    return pattern


def addTests(config, patterns):
    """Add all the kcov test modules.

    Discovery is not possible, since some modules need to be excluded,
    depending on the os and arch.
    """

    test_loader = TestLoader(config, patterns)

    test_loader.add_tests_from_module("test_basic")
    test_loader.add_tests_from_module("test_compiled_basic")

    if platform.machine() in ["x86_64", "i386", "i686"]:
        test_loader.add_tests_from_module("test_compiled")
    if sys.platform.startswith("linux"):
        test_loader.add_tests_from_module("test_bash_linux_only")

        if platform.machine() in ["x86_64", "i386", "i686"]:
            test_loader.add_tests_from_module("test_system_mode")

    test_loader.add_tests_from_module("test_accumulate")
    test_loader.add_tests_from_module("test_bash")
    test_loader.add_tests_from_module("test_filter")
    test_loader.add_tests_from_module("test_python")

    return test_loader.tests


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("kcov", type=normalized_not_empty)
    parser.add_argument("outbase", type=normalized_not_empty)
    parser.add_argument("testbuild", type=normalized_not_empty)
    parser.add_argument("sources", type=normalized_not_empty)

    # Code copied from unittest.main.
    parser.add_argument(
        "-v",
        "--verbose",
        dest="verbosity",
        action="store_const",
        const=2,
        help="Verbose output",
    )
    parser.add_argument(
        "-q",
        "--quiet",
        dest="verbosity",
        action="store_const",
        const=0,
        help="Quiet output",
    )
    parser.add_argument(
        "-f",
        "--failfast",
        dest="failfast",
        action="store_true",
        help="Stop on first fail or error",
    )
    parser.add_argument(
        "-b",
        "--buffer",
        dest="buffer",
        action="store_true",
        help="Buffer stdout and stderr during tests",
    )
    parser.add_argument(
        "-c",
        "--catch",
        dest="catchbreak",
        action="store_true",
        help="Catch Ctrl-C and display results so far",
    )
    parser.add_argument(
        "-k",
        dest="patterns",
        action="append",
        type=to_fnmatch,
        help="Only run tests which match the given substring",
    )
    parser.add_argument(
        "--locals",
        dest="tb_locals",
        action="store_true",
        help="Show local variables in tracebacks",
    )

    # TODO: The --duration argument was added in 3.12.

    # kcov test runner options

    parser.add_argument(
        "--shuffle",
        dest="shuffle",
        action="store_true",
        help="Randomize the execution order of tests",
    )

    # TODO: The --duration argument was added in 3.12.

    # kcov test runner custom options

    return parser.parse_args()


def main():
    # Parse the command line and validate the configuration paths
    args = parse_args()

    if os.path.abspath(args.outbase) == os.getcwd():
        fatal("'outbase' cannot be the current directory")

    # Loads and configure tests
    config = Config(args.kcov, args.outbase, args.testbuild, args.sources)
    tests = addTests(config, args.patterns)

    if args.shuffle:
        random.shuffle(tests)

    # Run the tests
    test_suite = unittest.TestSuite(tests)

    # Code copied from unittest.main.
    if args.catchbreak:
        unittest.installHandler()

    test_runner = unittest.TextTestRunner(
        verbosity=args.verbosity,
        failfast=args.failfast,
        buffer=args.buffer,
        tb_locals=args.tb_locals,
    )

    result = test_runner.run(test_suite)
    if True:
        if result.testsRun == 0 and len(result.skipped) == 0:
            sys.exit(_NO_TESTS_EXITCODE)
        elif result.wasSuccessful():
            sys.exit(0)
        else:
            sys.exit(1)
