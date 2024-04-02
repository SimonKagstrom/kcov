import unittest

import libkcov as testbase
from libkcov import cobertura


class python_exit_status(testbase.KcovTestCase):
    def runTest(self):
        noKcovRv, o = self.doCmd(self.sources + "/tests/python/main 5")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.sources + "/tests/python/main 5"
        )

        assert rv == noKcovRv


class python_can_set_illegal_parser(testbase.KcovTestCase):
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


class python_can_set_legal_parser(testbase.KcovTestCase):
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


class python2_can_set_legal_parser(testbase.KcovTestCase):
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


class python_issue_368_can_handle_symlink_target(testbase.KcovTestCase):
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


class python_unittest(testbase.KcovTestCase):
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


class PythonBase(testbase.KcovTestCase):
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
    def runTestTest(self):
        self.doTest("")


class python_accumulate_data(testbase.KcovTestCase):
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
    def runTest(self):
        self.doTest("--python-parser=python2")


class python_tricky_single_line_string_assignment(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.sources + "/tests/python/main 5"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "second.py", 34) == 2


class python_select_parser(testbase.KcovTestCase):
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


class python_tricky_single_dict_assignment(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.sources + "/tests/python/main 5"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "second.py", 57) == 1
        assert cobertura.hitsPerLine(dom, "second.py", 61) == 1
