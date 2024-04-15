import shutil
import unittest

import libkcov
from libkcov import cobertura

skip_python2 = shutil.which("python2") is None


class python_exit_status(libkcov.TestCase):
    def runTest(self):
        noKcovRv, o = self.doCmd(self.sources + "/tests/python/main 5")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.sources + "/tests/python/main 5"
        )

        assert rv == noKcovRv


class python_can_set_illegal_parser(libkcov.TestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --python-parser=python7 "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/python/main 5"
        )

        assert b"Cannot find Python parser 'python7'" in o


class python_can_set_legal_parser(libkcov.TestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --python-parser=python3 "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/python/main 5"
        )

        assert b"Cannot find Python parser 'python3'" not in o


class python2_can_set_legal_parser(libkcov.TestCase):
    @unittest.skipIf(skip_python2, "python2 not available")
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --python-parser=python2 "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/python/main 5"
        )

        assert b"Cannot find Python parser 'python2'" not in o


class python_issue_368_can_handle_symlink_target(libkcov.TestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --python-parser=python3 "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/python/link_main 5 --foo"
        )

        assert b"unrecognized option '--foo'" not in o


class python_unittest(libkcov.TestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/python/unittest/testdriver"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/testdriver/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "testdriver", 14) == 1


class PythonBase(libkcov.TestCase):
    def doTest(self, extra):
        rv, o = self.do(
            self.kcov
            + " "
            + extra
            + " "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/python/main 5"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main", 10) == 2
        assert cobertura.hitsPerLine(dom, "main", 17) == 0
        assert cobertura.hitsPerLine(dom, "main", 22) is None
        assert cobertura.hitsPerLine(dom, "second.py", 2) == 1
        assert cobertura.hitsPerLine(dom, "second.py", 4) is None
        assert cobertura.hitsPerLine(dom, "second.py", 11) is None
        assert cobertura.hitsPerLine(dom, "second.py", 31) == 0
        assert cobertura.hitsPerLine(dom, "second.py", 38) == 1
        assert cobertura.hitsPerLine(dom, "second.py", 39) == 0
        assert cobertura.hitsPerLine(dom, "second.py", 40) is None
        assert cobertura.hitsPerLine(dom, "second.py", 41) == 1
        assert cobertura.hitsPerLine(dom, "second.py", 56) is None
        assert cobertura.hitsPerLine(dom, "second.py", 60) is None
        assert cobertura.hitsPerLine(dom, "second.py", 65) is None
        assert cobertura.hitsPerLine(dom, "second.py", 77) is None


class python_coverage(PythonBase):
    def runTest(self):
        self.doTest("")


class python_accumulate_data(libkcov.TestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.sources + "/tests/python/main 5"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main", 16) == 0
        assert cobertura.hitsPerLine(dom, "main", 19) == 1
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.sources + "/tests/python/main"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main", 16) == 1
        assert cobertura.hitsPerLine(dom, "main", 19) == 1


class python3_coverage(PythonBase):
    def runTest(self):
        self.doTest("--python-parser=python3")


class python2_coverage(PythonBase):
    @unittest.skipIf(skip_python2, "python2 not available")
    def runTest(self):
        self.doTest("--python-parser=python2")


class python_tricky_single_line_string_assignment(libkcov.TestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.sources + "/tests/python/main 5"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "second.py", 34) == 2


class python_select_parser(libkcov.TestCase):
    def disabledTest(self):
        rv, o = self.do(
            self.kcov
            + " --python-parser="
            + self.sources
            + "/tests/tools/dummy-python.sh "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/python/main 5"
        )

        assert rv == 99


class python_tricky_single_dict_assignment(libkcov.TestCase):
    @unittest.expectedFailure
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.sources + "/tests/python/main 5"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "second.py", 57) == 1
        assert cobertura.hitsPerLine(dom, "second.py", 61) == 1
