import libkcov as testbase
from libkcov import cobertura


class bash_sh_shebang(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --bash-handle-sh-invocation "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/bash/shell-main"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/shell-main/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "sh-shebang.sh", 4) == 1


class bash_exit_before_child(testbase.KcovTestCase):
    def runTest(self):
        # kcovKcov shouldn't wait for the background process, so call it with kcovKcov = False
        rv, o = self.do(
            self.kcov
            + " --bash-tracefd-cloexec "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/bash/background-child.sh",
            kcovKcov=False,
            timeout=3.0,
        )
        self.assertEqual(0, rv, "kcov exited unsuccessfully")
        dom = cobertura.parseFile(self.outbase + "/kcov/background-child.sh/cobertura.xml")
        self.assertIsNone(cobertura.hitsPerLine(dom, "background-child.sh", 1))
        self.assertEqual(1, cobertura.hitsPerLine(dom, "background-child.sh", 3))
        self.assertEqual(1, cobertura.hitsPerLine(dom, "background-child.sh", 4))


class bash_ldpreload_multilib(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --bash-handle-sh-invocation --bash-tracefd-cloexec "
            + self.outbase
            + "/kcov "
            + self.sources
            + "/tests/bash/sh-shebang.sh"
        )
        self.assertEqual(0, rv, "kcov exited unsuccessfully")
        dom = cobertura.parseFile(self.outbase + "/kcov/sh-shebang.sh/cobertura.xml")
        self.assertIsNone(cobertura.hitsPerLine(dom, "sh-shebang.sh", 1))
        self.assertEqual(1, cobertura.hitsPerLine(dom, "sh-shebang.sh", 4))
