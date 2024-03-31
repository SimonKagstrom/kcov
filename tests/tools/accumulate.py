import cobertura
import testbase


class accumulate_data(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main"
        )

        dom = cobertura.parseFile(testbase.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main", 16) == 1
        assert cobertura.hitsPerLine(dom, "main", 19) == 0
        assert cobertura.hitsPerLine(dom, "main", 14) == 1

        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )
        dom = cobertura.parseFile(testbase.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main", 16) == 1
        assert cobertura.hitsPerLine(dom, "main", 19) == 1
        assert cobertura.hitsPerLine(dom, "main", 14) == 2


class dont_accumulate_data_with_clean(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main"
        )

        dom = cobertura.parseFile(testbase.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main", 16) == 1
        assert cobertura.hitsPerLine(dom, "main", 19) == 0
        assert cobertura.hitsPerLine(dom, "main", 14) == 1

        rv, o = self.do(
            testbase.kcov
            + " --clean "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )
        dom = cobertura.parseFile(testbase.outbase + "/kcov/main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main", 16) == 0
        assert cobertura.hitsPerLine(dom, "main", 19) == 1
        assert cobertura.hitsPerLine(dom, "main", 14) == 1


class merge_basic(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/python/main 5"
        )
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/bash/shell-main"
        )

        dom = cobertura.parseFile(testbase.outbase + "/kcov/kcov-merged/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main", 10) == 1
        assert cobertura.hitsPerLine(dom, "shell-main", 4) == 1


class merge_multiple_output_directories(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov/first "
            + testbase.sources
            + "/tests/python/main 5"
        )
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov/second "
            + testbase.sources
            + "/tests/bash/shell-main"
        )
        rv, o = self.do(
            testbase.kcov
            + " --merge "
            + testbase.outbase
            + "/kcov/merged "
            + testbase.outbase
            + "/kcov/first "
            + testbase.outbase
            + "/kcov/second"
        )
        dom = cobertura.parseFile(testbase.outbase + "/kcov/merged/kcov-merged/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main", 10) == 1
        assert cobertura.hitsPerLine(dom, "shell-main", 4) == 1


class merge_merged_output(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov/first "
            + testbase.sources
            + "/tests/python/main 5"
        )
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov/second "
            + testbase.sources
            + "/tests/bash/shell-main"
        )
        rv, o = self.do(
            testbase.kcov
            + " "
            + testbase.outbase
            + "/kcov/third "
            + testbase.sources
            + "/tests/bash/dollar-var-replacements.sh"
        )

        rv, o = self.do(
            testbase.kcov
            + " --merge "
            + testbase.outbase
            + "/kcov/merged "
            + testbase.outbase
            + "/kcov/first "
            + testbase.outbase
            + "/kcov/second"
        )
        rv, o = self.do(
            testbase.kcov
            + " --merge "
            + testbase.outbase
            + "/kcov/merged2 "
            + testbase.outbase
            + "/kcov/merged "
            + testbase.outbase
            + "/kcov/third"
        )
        dom = cobertura.parseFile(testbase.outbase + "/kcov/merged2/kcov-merged/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main", 10) == 1
        assert cobertura.hitsPerLine(dom, "shell-main", 4) == 1
        assert cobertura.hitsPerLine(dom, "dollar-var-replacements.sh", 2) == 1


class merge_coveralls(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            testbase.kcov
            + " --coveralls-id=dry-run "
            + testbase.outbase
            + "/kcov/ "
            + testbase.sources
            + "/tests/python/main 5"
        )
        rv, o = self.do(
            testbase.kcov
            + " --coveralls-id=dry-run "
            + testbase.outbase
            + "/kcov/ "
            + testbase.sources
            + "/tests/bash/shell-main"
        )

        rv, o = self.doShell(f"grep second.py {(testbase.outbase)}/kcov/main/coveralls.out")
        assert rv == 0
        rv, o = self.doShell(f"grep shell-main {(testbase.outbase)}/kcov/shell-main/coveralls.out")
        assert rv == 0
