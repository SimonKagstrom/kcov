import unittest

import parse_cobertura
import testbase


class python_exit_status(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        noKcovRv, o = self.do(testbase.sources + "/tests/python/main 5")
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )

        assert rv == noKcovRv


class python_can_set_illegal_parser(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " --python-parser=python7 "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )

        assert b"Cannot find Python parser 'python7'" in o


class python_can_set_legal_parser(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " --python-parser=python3 "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )

        assert b"Cannot find Python parser 'python3'" not in o


class python2_can_set_legal_parser(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " --python-parser=python2 "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )

        assert b"Cannot find Python parser 'python2'" not in o


class python_issue_368_can_handle_symlink_target(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " --python-parser=python3 "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/link_main 5 --foo"
        )

        assert b"unrecognized option '--foo'" not in o


class python_unittest(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/unittest/testdriver"
        )

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/testdriver/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "testdriver", 14) == 1


class PythonBase(testbase.KcovTestCase):
    def doTest(self, extra):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " "
            + extra
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main", 10) == 2
        assert parse_cobertura.hitsPerLine(dom, "main", 17) == 0
        assert parse_cobertura.hitsPerLine(dom, "main", 22) == None
        assert parse_cobertura.hitsPerLine(dom, "second.py", 2) == 1
        assert parse_cobertura.hitsPerLine(dom, "second.py", 4) == None
        assert parse_cobertura.hitsPerLine(dom, "second.py", 11) == None
        assert parse_cobertura.hitsPerLine(dom, "second.py", 31) == 0
        assert parse_cobertura.hitsPerLine(dom, "second.py", 38) == 1
        assert parse_cobertura.hitsPerLine(dom, "second.py", 39) == 0
        assert parse_cobertura.hitsPerLine(dom, "second.py", 40) == None
        assert parse_cobertura.hitsPerLine(dom, "second.py", 41) == 1
        assert parse_cobertura.hitsPerLine(dom, "second.py", 56) == None
        assert parse_cobertura.hitsPerLine(dom, "second.py", 60) == None
        assert parse_cobertura.hitsPerLine(dom, "second.py", 65) == None
        assert parse_cobertura.hitsPerLine(dom, "second.py", 77) == None


class python_coverage(PythonBase):
    def runTestTest(self):
        self.doTest("")


class python_accumulate_data(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main", 16) == 0
        assert parse_cobertura.hitsPerLine(dom, "main", 19) == 1
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main"
        )

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main", 16) == 1
        assert parse_cobertura.hitsPerLine(dom, "main", 19) == 1


class python3_coverage(PythonBase):
    def runTest(self):
        self.doTest("--python-parser=python3")


class python2_coverage(PythonBase):
    def runTest(self):
        self.doTest("--python-parser=python2")


class python_tricky_single_line_string_assignment(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "second.py", 34) == 2


class python_select_parser(testbase.KcovTestCase):
    def disabledTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " --python-parser="
            + testbase.sources
            + "/tests/tools/dummy-python.sh "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )

        assert rv == 99


class python_tricky_single_dict_assignment(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "second.py", 57) == 1
        assert parse_cobertura.hitsPerLine(dom, "second.py", 61) == 1
