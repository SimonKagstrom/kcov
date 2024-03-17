
import parse_cobertura
import testbase


class include_exclude_pattern(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " --bash-dont-parse-binary-dir --exclude-pattern=first-dir "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/bash/shell-main"
        )

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 29) >= 1
        assert parse_cobertura.hitsPerLine(dom, "c.sh", 3) == None

        rv, o = self.do(
            testbase.kcov
            + " --include-pattern=first-dir "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/bash/shell-main"
        )
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 29) == None
        assert parse_cobertura.hitsPerLine(dom, "c.sh", 3) >= 1

        rv, o = self.do(
            testbase.kcov
            + " --include-pattern=first-dir --exclude-pattern=c.sh "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/bash/shell-main"
        )
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 29) == None
        assert parse_cobertura.hitsPerLine(dom, "c.sh", 3) == None
        assert parse_cobertura.hitsPerLine(dom, "b.sh", 3) >= 1


class include_path(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " --bash-dont-parse-binary-dir --include-path="
            + testbase.sources
            + "/tests/bash/first-dir "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/bash/shell-main"
        )

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 29) == None
        assert parse_cobertura.hitsPerLine(dom, "c.sh", 3) >= 1
        assert parse_cobertura.hitsPerLine(dom, "b.sh", 3) >= 1


class include_path_and_exclude_pattern(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv, o = self.do(
            testbase.kcov
            + " --bash-dont-parse-binary-dir --include-path="
            + testbase.sources
            + "/tests/bash/first-dir --exclude-pattern=b.sh "
            + testbase.outbase
            + "/kcov "
            + testbase.sources
            + "/tests/bash/shell-main"
        )

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 29) == None
        assert parse_cobertura.hitsPerLine(dom, "c.sh", 3) >= 1
        assert parse_cobertura.hitsPerLine(dom, "b.sh", 3) == None
