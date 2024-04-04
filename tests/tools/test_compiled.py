import os
import platform
import sys
import unittest

import libkcov
from libkcov import cobertura


class illegal_insn(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    @unittest.skipUnless(platform.machine() in ["x86_64", "i686", "i386"], "Only for x86")
    def runTest(self):
        rv, output = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/illegal-insn",
            False,
        )
        assert rv != 0
        assert b"Illegal instructions are" in output


class fork_no_wait(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        noKcovRv, o = self.doCmd(self.binaries + "/fork_no_wait")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/fork_no_wait",
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/fork_no_wait/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "fork-no-wait.c", 20) >= 1
        assert cobertura.hitsPerLine(dom, "fork-no-wait.c", 22) >= 1
        assert cobertura.hitsPerLine(dom, "fork-no-wait.c", 24) >= 1


class ForkBase(libkcov.TestCase):
    def doTest(self, binary):
        noKcovRv, o = self.doCmd(self.binaries + "/" + binary)
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/" + binary,
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/" + binary + "/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "fork.c", 21) == 0
        assert cobertura.hitsPerLine(dom, "fork.c", 26) >= 1
        assert cobertura.hitsPerLine(dom, "fork.c", 34) >= 1
        assert cobertura.hitsPerLine(dom, "fork.c", 37) >= 1
        assert cobertura.hitsPerLine(dom, "fork.c", 46) >= 1


class fork_64(ForkBase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        self.doTest("fork")


class fork_32(ForkBase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    @unittest.skipUnless(platform.machine().startswith("x86_64"), "Only for x86_64")
    def runTest(self):
        self.skipTest("Fickle test, ignoring")
        self.doTest("fork-32")


class vfork(libkcov.TestCase):
    @unittest.skipIf(
        sys.platform.startswith("darwin"),
        "Not for OSX (does not work with the mach-engine for now)",
    )
    def runTest(self):
        rv, o = self.do(self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/vfork", False)
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/vfork/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "vfork.c", 12) >= 1
        assert cobertura.hitsPerLine(dom, "vfork.c", 18) >= 1


class popen_test(libkcov.TestCase):
    @unittest.skipUnless(platform.machine() in ["x86_64", "i686", "i386"], "Only for x86")
    def runTest(self):
        noKcovRv, o = self.doCmd(self.binaries + "/test_popen")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/test_popen",
            False,
        )
        assert rv == noKcovRv

        assert b"popen OK" in o


class short_filename(libkcov.TestCase):
    @unittest.expectedFailure
    def runTest(self):
        rv, o = self.do(self.kcov + " " + self.outbase + "/kcov ./s", False)
        assert rv == 99


class Pie(libkcov.TestCase):
    def runTest(self):
        noKcovRv, o = self.doCmd(self.binaries + "/pie")
        rv, o = self.do(self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/pie", False)
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/pie/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "pie.c", 5) == 1


class pie_argv_basic(libkcov.TestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/pie-test",
            False,
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/pie-test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 11) == 0


class pie_accumulate(libkcov.TestCase):
    @unittest.skipIf(
        sys.platform.startswith("darwin"),
        "Not for OSX (does not work with the mach-engine for now)",
    )
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/pie-test",
            False,
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/pie-test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 11) == 0

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/pie-test a",
            False,
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/pie-test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 11) == 1


class global_ctors(libkcov.TestCase):
    def runTest(self):
        noKcovRv, o = self.doCmd(self.binaries + "/global-constructors")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/global-constructors",
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/global-constructors/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "test-global-ctors.cc", 4) >= 1


class daemon_wait_for_last_child(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #158")
    @unittest.skipUnless(platform.machine() in ["x86_64", "i686", "i386"], "Only for x86")
    def runTest(self):
        noKcovRv, o = self.doCmd(self.binaries + "/test_daemon")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/test_daemon",
            False,
        )
        assert rv == 4
        assert noKcovRv == 2

        dom = cobertura.parseFile(self.outbase + "/kcov/test_daemon/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "test-daemon.cc", 31) == 1


class SignalsBase(libkcov.TestCase):
    def SignalsBase():
        self.m_self = ""

    def cmpOne(self, sig):
        noKcovRv, o = self.doCmd(self.binaries + "/signals " + sig + " " + self.m_self)
        rv, o = self.do(
            self.kcov
            + " "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/signals "
            + sig
            + " "
            + self.m_self,
            False,
        )
        assert rv == noKcovRv

        return cobertura.parseFile(self.outbase + "/kcov/signals/cobertura.xml")

    def doTest(self):
        dom = self.cmpOne("hup")
        assert cobertura.hitsPerLine(dom, "test-signals.c", 14) == 1

        dom = self.cmpOne("int")
        assert cobertura.hitsPerLine(dom, "test-signals.c", 19) == 1

        dom = self.cmpOne("quit")
        assert cobertura.hitsPerLine(dom, "test-signals.c", 24) == 1

        dom = self.cmpOne("bus")
        assert cobertura.hitsPerLine(dom, "test-signals.c", 44) == 1

        dom = self.cmpOne("fpe")
        assert cobertura.hitsPerLine(dom, "test-signals.c", 49) == 1

        dom = self.cmpOne("term")
        assert cobertura.hitsPerLine(dom, "test-signals.c", 84) == 1


class signals(SignalsBase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #158")
    def runTest(self):
        self.m_self = ""
        self.doTest()


class signals_self(SignalsBase):
    def runTest(self):
        self.m_self = "self"
        self.doTest()


class signals_crash(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX (macho-parser for now)")
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/signals segv self",
            False,
        )
        assert b"kcov: Process exited with signal 11" in o

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/signals abrt self",
            False,
        )
        assert b"kcov: Process exited with signal 6" in o


class collect_and_report_only(libkcov.TestCase):
    # Cannot work with combined Engine / Parser
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        noKcovRv, o = self.doCmd(self.binaries + "/main-tests ")
        rv, o = self.do(
            self.kcov
            + " --collect-only "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/main-tests",
            False,
        )
        self.skipTest("Fickle test, ignoring")
        assert rv == noKcovRv

        try:
            dom = cobertura.parseFile(self.outbase + "/kcov/main-tests/cobertura.xml")
            self.fail("File unexpectedly found")
        except:
            # Exception is expected here
            pass

        rv, o = self.do(
            self.kcov + " --report-only " + self.outbase + "/kcov " + self.binaries + "/main-tests",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1


class setpgid_kill(libkcov.TestCase):
    def runTest(self):
        noKcovRv, o = self.doCmd(
            self.sources + "/tests/setpgid-kill/test-script.sh " + self.binaries + "/setpgid-kill"
        )
        assert b"SUCCESS" in o

        rv, o = self.do(
            self.sources
            + "/tests/setpgid-kill/test-script.sh "
            + self.kcov
            + " "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/setpgid-kill",
            False,
        )
        assert b"SUCCESS" in o


class attach_process_with_threads(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        rv, o = self.do(
            self.sources
            + "/tests/daemon/test-script.sh "
            + self.kcov
            + " "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/issue31",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/issue31/cobertura.xml")
        self.skipTest("Fickle test, ignoring")
        assert cobertura.hitsPerLine(dom, "test-issue31.cc", 28) >= 1
        assert cobertura.hitsPerLine(dom, "test-issue31.cc", 11) >= 1
        assert cobertura.hitsPerLine(dom, "test-issue31.cc", 9) == 0


class attach_process_with_threads_creates_threads(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        rv, o = self.do(
            self.sources
            + "/tests/daemon/test-script.sh "
            + self.kcov
            + " "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/thread-test",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/thread-test/cobertura.xml")
        self.skipTest("Fickle test, ignoring")
        assert cobertura.hitsPerLine(dom, "thread-main.c", 21) >= 1
        assert cobertura.hitsPerLine(dom, "thread-main.c", 9) >= 1


class merge_same_file_in_multiple_binaries(libkcov.TestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/multi_1",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/multi_1/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main_1.c", 10) == 0
        assert cobertura.hitsPerLine(dom, "file.c", 3) == 0

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/multi_2",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/kcov-merged/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main_2.c", 9) == 1
        assert cobertura.hitsPerLine(dom, "file.c", 3) == 0
        assert cobertura.hitsPerLine(dom, "file.c", 8) == 0

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/multi_2 1",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/kcov-merged/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "file.c", 3) == 0
        assert cobertura.hitsPerLine(dom, "file.c", 8) == 1


class debuglink(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        os.system(f"rm -rf {(self.outbase)}/.debug")
        os.system(f"cp {self.binaries}/main-tests {self.binaries}/main-tests-debug-file")
        os.system(
            f"objcopy --only-keep-debug {self.binaries}/main-tests-debug-file {self.binaries}/main-tests-debug-file.debug"
        )
        os.system(
            f"cp {self.binaries}/main-tests-debug-file.debug {self.binaries}/main-tests-debug-file1.debug"
        )
        os.system(
            f"cp {self.binaries}/main-tests-debug-file.debug {self.binaries}/main-tests-debug-file12.debug"
        )
        os.system(
            f"cp {self.binaries}/main-tests-debug-file.debug {self.binaries}/main-tests-debug-file123.debug"
        )
        os.system(
            f"cp {self.binaries}/main-tests-debug-file {self.binaries}/main-tests-debug-file1"
        )
        os.system(
            f"cp {self.binaries}/main-tests-debug-file {self.binaries}/main-tests-debug-file2"
        )
        os.system(
            f"cp {self.binaries}/main-tests-debug-file {self.binaries}/main-tests-debug-file3"
        )
        os.system(f"strip -g {(self.binaries)}/main-tests-debug-file")
        os.system(f"strip -g {(self.binaries)}/main-tests-debug-file1")
        os.system(f"strip -g {(self.binaries)}/main-tests-debug-file2")
        os.system(f"strip -g {(self.binaries)}/main-tests-debug-file3")
        os.system(
            f"objcopy --add-gnu-debuglink={self.binaries}/main-tests-debug-file.debug {self.binaries}/main-tests-debug-file"
        )
        os.system(
            f"objcopy --add-gnu-debuglink={self.binaries}/main-tests-debug-file1.debug {self.binaries}/main-tests-debug-file1"
        )
        os.system(
            f"objcopy --add-gnu-debuglink={self.binaries}/main-tests-debug-file12.debug {self.binaries}/main-tests-debug-file2"
        )
        os.system(
            f"objcopy --add-gnu-debuglink={self.binaries}/main-tests-debug-file123.debug {self.binaries}/main-tests-debug-file3"
        )

        noKcovRv, o = self.doCmd(self.binaries + "/main-tests-debug-file")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/main-tests-debug-file",
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        # Check alignment
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/main-tests-debug-file1",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file1/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/main-tests-debug-file2",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file2/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/main-tests-debug-file3",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file3/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        # Look in .debug
        os.system(f"rm -rf {(self.outbase)}/kcov")
        os.system(f"mkdir -p {(self.binaries)}/.debug")
        os.system(f"mv {self.binaries}/main-tests-debug-file.debug {self.binaries}/.debug")

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/main-tests-debug-file",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        os.system(f"rm -rf {(self.outbase)}/kcov")
        os.system(f"echo 'abc' >> {(self.binaries)}/.debug/main-tests-debug-file.debug")

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/main-tests-debug-file",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) is None


# Todo: Look in /usr/lib/debug as well


class collect_no_source(libkcov.TestCase):
    @unittest.expectedFailure
    def runTest(self):
        os.system(f"cp {self.sources}/tests/short-file.c {self.binaries}/main.cc")
        os.system(f"gcc -g -o {self.binaries}/main-collect-only {self.binaries}/main.cc")
        os.system(f"mv {self.binaries}/main.cc {self.binaries}/tmp-main.cc")

        rv, o = self.do(
            self.kcov
            + " --collect-only "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/main-collect-only",
            False,
        )

        os.system(f"mv {self.binaries}/tmp-main.cc {self.binaries}/main.cc")
        rv, o = self.do(
            self.kcov
            + " --report-only "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/main-collect-only"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/main-collect-only/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 1) == 1


class dlopen(libkcov.TestCase):
    @unittest.expectedFailure
    def runTest(self):
        noKcovRv, o = self.doCmd(self.binaries + "/dlopen")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.binaries + "/dlopen",
            False,
        )

        assert noKcovRv == rv
        dom = cobertura.parseFile(self.outbase + "/kcov/dlopen/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "dlopen.cc", 11) == 1
        assert cobertura.hitsPerLine(dom, "dlopen.cc", 12) == 0
        assert cobertura.hitsPerLine(dom, "solib.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 12) == 0


class dlopen_in_ignored_source_file(libkcov.TestCase):
    @unittest.expectedFailure
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --exclude-pattern=dlopen.cc "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/dlopen",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/dlopen/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "dlopen-main.cc", 10) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 12) == 0


class daemon_no_wait_for_last_child(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    @unittest.expectedFailure
    def runTest(self):
        noKcovRv, o = self.doCmd(self.binaries + "/test_daemon")
        rv, o = self.do(
            self.kcov
            + " --output-interval=1 --exit-first-process "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/test_daemon",
            False,
        )

        assert noKcovRv == rv

        time.sleep(2)
        dom = cobertura.parseFile(self.outbase + "/kcov/test_daemon/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "test-daemon.cc", 31) == 0

        time.sleep(5)
        dom = cobertura.parseFile(self.outbase + "/kcov/test_daemon/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "test-daemon.cc", 31) == 1


class address_sanitizer_coverage(libkcov.TestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    @unittest.expectedFailure
    def runTest(self):
        if not os.path.isfile(self.binaries + "/sanitizer-coverage"):
            self.write_message("Clang-only")
            assert False
        rv, o = self.do(
            self.kcov
            + " --clang "
            + self.outbase
            + "/kcov "
            + self.binaries
            + "/sanitizer-coverage",
            False,
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/sanitizer-coverage/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "sanitizer-coverage.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "sanitizer-coverage.c", 7) == 1
        assert cobertura.hitsPerLine(dom, "sanitizer-coverage.c", 8) == 1

        assert cobertura.hitsPerLine(dom, "sanitizer-coverage.c", 16) == 1
        assert cobertura.hitsPerLine(dom, "sanitizer-coverage.c", 18) == 1

        assert cobertura.hitsPerLine(dom, "sanitizer-coverage.c", 22) == 0
        assert cobertura.hitsPerLine(dom, "sanitizer-coverage.c", 25) == 0
