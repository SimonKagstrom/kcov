import testbase
import os
import unittest
import parse_cobertura

class bash_sh_shebang(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " --bash-handle-sh-invocation " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/shell-main")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "sh-shebang.sh", 4) == 1
