import sys
import unittest

import parse_cobertura
import testbase


class shared_library(testbase.KcovTestCase):
    @unittest.skipIf(
        sys.platform.startswith("darwin"),
        "Not for OSX (does not work with the mach-engine for now)",
    )
    def runTest(self):
        noKcovRv, o = self.do(testbase.testbuild + "/shared_library_test", False)
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.testbuild
            + "/shared_library_test",
            False,
        )
        assert rv == noKcovRv

        dom = parse_cobertura.parseFile(
            testbase.outbase + "/kcov/shared_library_test/cobertura.xml"
        )
        assert parse_cobertura.hitsPerLine(dom, "main.c", 9) >= 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 5) == 1


class shared_library_skip(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #157")
    def runTest(self):
        rv, o = self.do(
            testbase.kcov
            + " --skip-solibs "
            + testbase.outbase
            + "/kcov "
            + testbase.testbuild
            + "/shared_library_test",
            False,
        )
        print("Fickle test, ignoring")
        return
        assert rv == 0

        dom = parse_cobertura.parseFile(
            testbase.outbase + "/kcov/shared_library_test/cobertura.xml"
        )
        assert parse_cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 5) is None


class shared_library_filter_out(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #157")
    def runTest(self):
        rv, o = self.do(
            testbase.kcov
            + " --exclude-pattern=solib "
            + testbase.outbase
            + "/kcov "
            + testbase.testbuild
            + "/shared_library_test",
            False,
        )
        assert rv == 0

        dom = parse_cobertura.parseFile(
            testbase.outbase + "/kcov/shared_library_test/cobertura.xml"
        )
        assert parse_cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 5) is None


class shared_library_accumulate(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #157")
    def runTest(self):
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.testbuild
            + "/shared_library_test 5",
            False,
        )
        assert rv == 0

        dom = parse_cobertura.parseFile(
            testbase.outbase + "/kcov/shared_library_test/cobertura.xml"
        )
        assert parse_cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 5) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 10) == 1


class MainTestBase(testbase.KcovTestCase):
    def doTest(self, verify):

        noKcovRv, o = self.do(testbase.testbuild + "/main-tests", False)
        rv, o = self.do(
            testbase.kcov
            + " "
            + verify
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.testbuild
            + "/main-tests 5",
            False,
        )
        assert rv == noKcovRv

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-tests/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 9) == 1
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 14) is None
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 18) >= 1
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 25) == 1
        assert parse_cobertura.hitsPerLine(dom, "file.c", 6) >= 1
        assert parse_cobertura.hitsPerLine(dom, "file2.c", 7) == 0


class main_test(MainTestBase):
    def runTest(self):
        self.doTest("")


class main_test_verify(MainTestBase):
    def runTest(self):
        self.doTest("--verify")


class main_test_lldb_raw_breakpoints(MainTestBase):
    def runTest(self):
        self.doTest("--configure=lldb-use-raw-breakpoint-writes=1")
