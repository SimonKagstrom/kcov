import os
import platform
import sys
import unittest

import cobertura
import testbase


class illegal_insn(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    @unittest.skipUnless(platform.machine() in ["x86_64", "i686", "i386"], "Only for x86")
    def runTest(self):
        rv, output = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/illegal-insn",
            False,
        )
        assert rv != 0
        assert b"Illegal instructions are" in output


class fork_no_wait(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        noKcovRv, o = self.doCmd(self.testbuild + "/fork_no_wait")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/fork_no_wait",
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/fork_no_wait/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "fork-no-wait.c", 20) >= 1
        assert cobertura.hitsPerLine(dom, "fork-no-wait.c", 22) >= 1
        assert cobertura.hitsPerLine(dom, "fork-no-wait.c", 24) >= 1


class ForkBase(testbase.KcovTestCase):
    def doTest(self, binary):
        noKcovRv, o = self.doCmd(self.testbuild + "/" + binary)
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/" + binary,
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


class vfork(testbase.KcovTestCase):
    @unittest.skipIf(
        sys.platform.startswith("darwin"),
        "Not for OSX (does not work with the mach-engine for now)",
    )
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/vfork", False
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/vfork/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "vfork.c", 12) >= 1
        assert cobertura.hitsPerLine(dom, "vfork.c", 18) >= 1


class popen_test(testbase.KcovTestCase):
    @unittest.skipUnless(platform.machine() in ["x86_64", "i686", "i386"], "Only for x86")
    def runTest(self):
        noKcovRv, o = self.doCmd(self.testbuild + "/test_popen")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/test_popen",
            False,
        )
        assert rv == noKcovRv

        assert b"popen OK" in o


class short_filename(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        rv, o = self.do(self.kcov + " " + self.outbase + "/kcov ./s", False)
        assert rv == 99


class Pie(testbase.KcovTestCase):
    def runTest(self):
        noKcovRv, o = self.doCmd(self.testbuild + "/pie")
        rv, o = self.do(self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/pie", False)
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/pie/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "pie.c", 5) == 1


class pie_argv_basic(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/pie-test",
            False,
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/pie-test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 11) == 0


class pie_accumulate(testbase.KcovTestCase):
    @unittest.skipIf(
        sys.platform.startswith("darwin"),
        "Not for OSX (does not work with the mach-engine for now)",
    )
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/pie-test",
            False,
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/pie-test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 11) == 0

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/pie-test a",
            False,
        )
        assert rv == 0

        dom = cobertura.parseFile(self.outbase + "/kcov/pie-test/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "argv-dependent.c", 11) == 1


class global_ctors(testbase.KcovTestCase):
    def runTest(self):
        noKcovRv, o = self.doCmd(self.testbuild + "/global-constructors")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/global-constructors",
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/global-constructors/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "test-global-ctors.cc", 4) >= 1


class daemon_wait_for_last_child(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX, Issue #158")
    @unittest.skipUnless(platform.machine() in ["x86_64", "i686", "i386"], "Only for x86")
    def runTest(self):
        noKcovRv, o = self.doCmd(self.testbuild + "/test_daemon")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/test_daemon",
            False,
        )
        assert rv == 4
        assert noKcovRv == 2

        dom = cobertura.parseFile(self.outbase + "/kcov/test_daemon/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "test-daemon.cc", 31) == 1


class SignalsBase(testbase.KcovTestCase):
    def SignalsBase():
        self.m_self = ""

    def cmpOne(self, sig):
        noKcovRv, o = self.doCmd(self.testbuild + "/signals " + sig + " " + self.m_self)
        rv, o = self.do(
            self.kcov
            + " "
            + self.outbase
            + "/kcov "
            + self.testbuild
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


class signals_crash(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX (macho-parser for now)")
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/signals segv self",
            False,
        )
        assert b"kcov: Process exited with signal 11" in o

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/signals abrt self",
            False,
        )
        assert b"kcov: Process exited with signal 6" in o


class collect_and_report_only(testbase.KcovTestCase):
    # Cannot work with combined Engine / Parser
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        noKcovRv, o = self.doCmd(self.testbuild + "/main-tests ")
        rv, o = self.do(
            self.kcov
            + " --collect-only "
            + self.outbase
            + "/kcov "
            + self.testbuild
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
            self.kcov
            + " --report-only "
            + self.outbase
            + "/kcov "
            + self.testbuild
            + "/main-tests",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1


class setpgid_kill(testbase.KcovTestCase):
    def runTest(self):
        noKcovRv, o = self.doCmd(
            self.sources + "/tests/setpgid-kill/test-script.sh " + self.testbuild + "/setpgid-kill"
        )
        assert b"SUCCESS" in o

        rv, o = self.do(
            self.sources
            + "/tests/setpgid-kill/test-script.sh "
            + self.kcov
            + " "
            + self.outbase
            + "/kcov "
            + self.testbuild
            + "/setpgid-kill",
            False,
        )
        assert b"SUCCESS" in o


class attach_process_with_threads(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        rv, o = self.do(
            self.sources
            + "/tests/daemon/test-script.sh "
            + self.kcov
            + " "
            + self.outbase
            + "/kcov "
            + self.testbuild
            + "/issue31",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/issue31/cobertura.xml")
        self.skipTest("Fickle test, ignoring")
        assert cobertura.hitsPerLine(dom, "test-issue31.cc", 28) >= 1
        assert cobertura.hitsPerLine(dom, "test-issue31.cc", 11) >= 1
        assert cobertura.hitsPerLine(dom, "test-issue31.cc", 9) == 0


class attach_process_with_threads_creates_threads(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        rv, o = self.do(
            self.sources
            + "/tests/daemon/test-script.sh "
            + self.kcov
            + " "
            + self.outbase
            + "/kcov "
            + self.testbuild
            + "/thread-test",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/thread-test/cobertura.xml")
        self.skipTest("Fickle test, ignoring")
        assert cobertura.hitsPerLine(dom, "thread-main.c", 21) >= 1
        assert cobertura.hitsPerLine(dom, "thread-main.c", 9) >= 1


class merge_same_file_in_multiple_binaries(testbase.KcovTestCase):
    def runTest(self):
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/multi_1",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/multi_1/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main_1.c", 10) == 0
        assert cobertura.hitsPerLine(dom, "file.c", 3) == 0

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/multi_2",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/kcov-merged/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main_2.c", 9) == 1
        assert cobertura.hitsPerLine(dom, "file.c", 3) == 0
        assert cobertura.hitsPerLine(dom, "file.c", 8) == 0

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/multi_2 1",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/kcov-merged/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "file.c", 3) == 0
        assert cobertura.hitsPerLine(dom, "file.c", 8) == 1


class debuglink(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    def runTest(self):
        os.system(f"rm -rf {(self.outbase)}/.debug")
        os.system(f"cp {self.testbuild}/main-tests {self.testbuild}/main-tests-debug-file")
        os.system(
            f"objcopy --only-keep-debug {self.testbuild}/main-tests-debug-file {self.testbuild}/main-tests-debug-file.debug"
        )
        os.system(
            f"cp {self.testbuild}/main-tests-debug-file.debug {self.testbuild}/main-tests-debug-file1.debug"
        )
        os.system(
            f"cp {self.testbuild}/main-tests-debug-file.debug {self.testbuild}/main-tests-debug-file12.debug"
        )
        os.system(
            f"cp {self.testbuild}/main-tests-debug-file.debug {self.testbuild}/main-tests-debug-file123.debug"
        )
        os.system(
            f"cp {self.testbuild}/main-tests-debug-file {self.testbuild}/main-tests-debug-file1"
        )
        os.system(
            f"cp {self.testbuild}/main-tests-debug-file {self.testbuild}/main-tests-debug-file2"
        )
        os.system(
            f"cp {self.testbuild}/main-tests-debug-file {self.testbuild}/main-tests-debug-file3"
        )
        os.system(f"strip -g {(self.testbuild)}/main-tests-debug-file")
        os.system(f"strip -g {(self.testbuild)}/main-tests-debug-file1")
        os.system(f"strip -g {(self.testbuild)}/main-tests-debug-file2")
        os.system(f"strip -g {(self.testbuild)}/main-tests-debug-file3")
        os.system(
            f"objcopy --add-gnu-debuglink={self.testbuild}/main-tests-debug-file.debug {self.testbuild}/main-tests-debug-file"
        )
        os.system(
            f"objcopy --add-gnu-debuglink={self.testbuild}/main-tests-debug-file1.debug {self.testbuild}/main-tests-debug-file1"
        )
        os.system(
            f"objcopy --add-gnu-debuglink={self.testbuild}/main-tests-debug-file12.debug {self.testbuild}/main-tests-debug-file2"
        )
        os.system(
            f"objcopy --add-gnu-debuglink={self.testbuild}/main-tests-debug-file123.debug {self.testbuild}/main-tests-debug-file3"
        )

        noKcovRv, o = self.doCmd(self.testbuild + "/main-tests-debug-file")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/main-tests-debug-file",
            False,
        )
        assert rv == noKcovRv

        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        # Check alignment
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/main-tests-debug-file1",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file1/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/main-tests-debug-file2",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file2/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/main-tests-debug-file3",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file3/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        # Look in .debug
        os.system(f"rm -rf {(self.outbase)}/kcov")
        os.system(f"mkdir -p {(self.testbuild)}/.debug")
        os.system(f"mv {self.testbuild}/main-tests-debug-file.debug {self.testbuild}/.debug")

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/main-tests-debug-file",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        os.system(f"rm -rf {(self.outbase)}/kcov")
        os.system(f"echo 'abc' >> {(self.testbuild)}/.debug/main-tests-debug-file.debug")

        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/main-tests-debug-file",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/main-tests-debug-file/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 9) is None


# Todo: Look in /usr/lib/debug as well


class collect_no_source(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        os.system(f"cp {self.sources}/tests/short-file.c {self.testbuild}/main.cc")
        os.system(f"gcc -g -o {self.testbuild}/main-collect-only {self.testbuild}/main.cc")
        os.system(f"mv {self.testbuild}/main.cc {self.testbuild}/tmp-main.cc")

        rv, o = self.do(
            self.kcov
            + " --collect-only "
            + self.outbase
            + "/kcov "
            + self.testbuild
            + "/main-collect-only",
            False,
        )

        os.system(f"mv {self.testbuild}/tmp-main.cc {self.testbuild}/main.cc")
        rv, o = self.do(
            self.kcov
            + " --report-only "
            + self.outbase
            + "/kcov "
            + self.testbuild
            + "/main-collect-only"
        )

        dom = cobertura.parseFile(self.outbase + "/kcov/main-collect-only/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "main.cc", 1) == 1


class dlopen(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        noKcovRv, o = self.doCmd(self.testbuild + "/dlopen")
        rv, o = self.do(
            self.kcov + " " + self.outbase + "/kcov " + self.testbuild + "/dlopen",
            False,
        )

        assert noKcovRv == rv
        dom = cobertura.parseFile(self.outbase + "/kcov/dlopen/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "dlopen.cc", 11) == 1
        assert cobertura.hitsPerLine(dom, "dlopen.cc", 12) == 0
        assert cobertura.hitsPerLine(dom, "solib.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 12) == 0


class dlopen_in_ignored_source_file(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        rv, o = self.do(
            self.kcov
            + " --exclude-pattern=dlopen.cc "
            + self.outbase
            + "/kcov "
            + self.testbuild
            + "/dlopen",
            False,
        )
        dom = cobertura.parseFile(self.outbase + "/kcov/dlopen/cobertura.xml")
        assert cobertura.hitsPerLine(dom, "dlopen-main.cc", 10) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 5) == 1
        assert cobertura.hitsPerLine(dom, "solib.c", 12) == 0


class daemon_no_wait_for_last_child(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    @unittest.expectedFailure
    def runTest(self):
        noKcovRv, o = self.doCmd(self.testbuild + "/test_daemon")
        rv, o = self.do(
            self.kcov
            + " --output-interval=1 --exit-first-process "
            + self.outbase
            + "/kcov "
            + self.testbuild
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


class address_sanitizer_coverage(testbase.KcovTestCase):
    @unittest.skipIf(sys.platform.startswith("darwin"), "Not for OSX")
    @unittest.expectedFailure
    def runTest(self):
        if not os.path.isfile(self.testbuild + "/sanitizer-coverage"):
            self.write_message("Clang-only")
            assert False
        rv, o = self.do(
            self.kcov
            + " --clang "
            + self.outbase
            + "/kcov "
            + self.testbuild
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
