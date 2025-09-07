import sys
import unittest

import libkcov
from libkcov import cobertura


class shared_library(libkcov.TestCase):
    @unittest.skipIf(
        sys.platform.startswith("darwin"),
        "Not for OSX (does not work with the mach-engine for now)",
    )
    def runTest(self):
        noKcovRv, o = self.doCmd(self.binaries + "/shared_library_test")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/shared_library_test",
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.c", 9) >= 1
        assert cobertura.hitsPerLine(dom, "solib.c", 5) == 1


class shared_library_skip(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #157")
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --skip-solibs "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/shared_library_test",
            False,
        )
        # Fickle since the binary is built as a PIE by default, not sure how to disable it
        self.skipTest("Fickle test, ignoring")
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.c", 9) >= 1
        assert cobertura.hitsPerLine(dom, "solib.c", 5) is None


class shared_library_filter_out(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #157")
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --exclude-pattern=solib "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/shared_library_test",
            False,
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 5) is None


class shared_library_accumulate(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #157")
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/shared_library_test 5",
            False,
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 11) >= 1


class MainTestBase(libkcov.TestCase):
    def doTest(self, verify):
        noKcovRv, o = self.doCmd(self.binaries + "/main-tests")
        rv, o = self.do(
            self.kcov
            + " "
            + verify
            + " "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/main-tests 5",
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1
        assert cobertura.hitsPerLine(dom, "main.cc", 14) is None
        # This is the location of a globally constructed object, so might or might not be hit
        # assert cobertura.hitsPerLine(dom, "main.cc", 18) >= 1
        assert cobertura.hitsPerLine(dom, "main.cc", 25) == 1
        assert cobertura.hitsPerLine(dom, "file.c", 6) >= 1
        assert cobertura.hitsPerLine(dom, "file2.c", 7) == 0


class main_test(MainTestBase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #485")
    def runTest(self):
        self.doTest("")


class main_test_verify(MainTestBase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #485")
    def runTest(self):
        self.doTest("--verify")
