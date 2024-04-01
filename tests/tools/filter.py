import cobertura
import testbase


class include_exclude_pattern(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --bash-dont-parse-binary-dir --exclude-pattern=first-dir "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/bash/shell-main"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/shell-main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "shell-main", 29) >= 1
        assert cobertura.hitsPerLine(dom, "c.sh", 3) is None

        rv, o = self.do(
            self.kcov
            + " --include-pattern=first-dir "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/bash/shell-main"
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/shell-main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "shell-main", 29) is None
        assert cobertura.hitsPerLine(dom, "c.sh", 3) >= 1

        rv, o = self.do(
            self.kcov
            + " --include-pattern=first-dir --exclude-pattern=c.sh "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/bash/shell-main"
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/shell-main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "shell-main", 29) is None
        assert cobertura.hitsPerLine(dom, "c.sh", 3) is None
        assert cobertura.hitsPerLine(dom, "b.sh", 3) >= 1


class include_path(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --bash-dont-parse-binary-dir --include-path="
            + self.sources
            + "/tests/bash/first-dir "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/bash/shell-main"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/shell-main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "shell-main", 29) is None
        assert cobertura.hitsPerLine(dom, "c.sh", 3) >= 1
        assert cobertura.hitsPerLine(dom, "b.sh", 3) >= 1


class include_path_and_exclude_pattern(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --bash-dont-parse-binary-dir --include-path="
            + self.sources
            + "/tests/bash/first-dir --exclude-pattern=b.sh "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/bash/shell-main"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/shell-main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "shell-main", 29) is None
        assert cobertura.hitsPerLine(dom, "c.sh", 3) >= 1
        assert cobertura.hitsPerLine(dom, "b.sh", 3) is None
