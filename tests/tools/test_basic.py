import os
import unittest

import libkcov
from libkcov import cobertura


class too_few_arguments(libkcov.TestCase):
    def runTest(self):
        rv, output = self.do(self.kcov + " " + self.outbase + "/kcov")

        assert b"Usage: kcov" in output
        assert rv == 1


class wrong_arguments(libkcov.TestCase):
    def runTest(self):
        rv, output = self.do(
            self.kcov + " --abc=efg " + self.outbase + "/kcov " + self.binaries + "/tests-stripped"
        )

        assert b"kcov: error: Unrecognized option: --abc=efg" in output
        assert rv == 1


class lookup_binary_in_path(libkcov.TestCase):
    @unittest.expectedFailure
    def runTest(self):
        os.environ["PATH"] += self.sources + "/tests/python"
        noKcovRv, o = self.do(self.sources + "/tests/python/main 5")
        rv, o = self.do(self.kcov + " " + self.outbase + "/kcov " + "main 5")

        dom = cobertura.parseFile(self.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "second.py", 34) == 2
        assert noKcovRv, rv


# Issue #414
class outdir_is_executable(libkcov.TestCase):
    def runTest(self):
        # Running a system executable on Linux may cause ptrace to fails with
        # "Operation not permitted", even with ptrace_scope set to 0.
        # See https://www.kernel.org/doc/Documentation/security/Yama.txt
        executable = self.sources + "/tests/python/short-test.py"
        rv, o = self.do(self.kcov + " echo " + executable)

        assert rv == 0
