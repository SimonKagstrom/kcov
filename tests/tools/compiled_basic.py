import sys
import unittest

import cobertura
import testbase


class shared_library(testbase.KcovTestCase):
    @unittest.skipIf(
        sys.platform.startswith("darwin"),
        "Not for OSX (does not work with the mach-engine for now)",
    )
    def runTest(self):
        noKcovRv, o = self.doCmd(self.testbuild + "/shared_library_test")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/shared_library_test",
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.c", 9) >= 1
        assert cobertura.hitsPerLine(dom, "solib.c", 5) == 1


class shared_library_skip(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #157")
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --skip-solibs "
            + self.outbase
            + "/kcov "
            + self.testbuild
            + "/shared_library_test",
            False,
        )
        self.skipTest("Fickle test, ignoring")
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 5) is None


class shared_library_filter_out(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #157")
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --exclude-pattern=solib "
            + self.outbase
            + "/kcov "
            + self.testbuild
            + "/shared_library_test",
            False,
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 5) is None


class shared_library_accumulate(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #157")
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/shared_library_test 5",
            False,
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 10) == 1


class MainTestBase(testbase.KcovTestCase):
    def doTest(self, verify):
        noKcovRv, o = self.doCmd(self.testbuild + "/main-tests")
        rv, o = self.do(
            self.kcov
            + " "
            + verify
            + " "
            + self.outbase
            + "/kcov "
            + self.testbuild
            + "/main-tests 5",
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1
        assert cobertura.hitsPerLine(dom, "main.cc", 14) is None
        assert cobertura.hitsPerLine(dom, "main.cc", 18) >= 1
        assert cobertura.hitsPerLine(dom, "main.cc", 25) == 1
        assert cobertura.hitsPerLine(dom, "file.c", 6) >= 1
        assert cobertura.hitsPerLine(dom, "file2.c", 7) == 0


class main_test(MainTestBase):
    def runTest(self):
        self.doTest("")


class main_test_verify(MainTestBase):
    def runTest(self):
        self.doTest("--verify")


class main_test_lldb_raw_breakpoints(MainTestBase):
    def runTest(self):
        self.doTest("--configure=lldb-use-raw-breakpoint-writes=1")
