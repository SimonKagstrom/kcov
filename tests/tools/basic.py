import testbase
import unittest
import parse_cobertura
import os

class TooFewArguments(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, output = self.do(testbase.kcov + " " + testbase.outbase + "/kcov")
        assert rv == 1

class WrongArguments(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, output = self.do(testbase.kcov + " --abc=efg " + testbase.outbase + "/kcov " + testbase.testbuild + "/tests-stripped")
        assert rv == 1

class LookupBinaryInPath(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()
        os.environ["PATH"] += testbase.sources + "/tests/python"
        noKcovRv,o = self.do(testbase.sources + "/tests/python/main 5")
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + "main 5")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "second.py", 34) == 2
        assert noKcovRv, rv

# Issue #414
class OutDirectoryIsExecutable(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        kcov_path = os.path.realpath(testbase.kcov)
        os.chdir(os.path.realpath(testbase.outbase))
        rv,o = self.do(kcov_path + " echo python -V")

        assert rv == 0
