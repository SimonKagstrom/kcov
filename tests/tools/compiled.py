import testbase
import unittest
import parse_cobertura
import sys
import os

class illegal_insn(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only")
    def runTest(self):
        self.setUp()
        rv, output = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/illegal-insn", False)
        assert rv != 0
        assert output.find("Illegal instructions are") != -1

class fork_no_wait(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only")
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.testbuild + "/fork_no_wait", False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/fork_no_wait", False)
        assert rv == noKcovRv

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/fork_no_wait/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "fork-no-wait.c", 20) >= 1
        assert parse_cobertura.hitsPerLine(dom, "fork-no-wait.c", 22) >= 1
        assert parse_cobertura.hitsPerLine(dom, "fork-no-wait.c", 24) >= 1

class ForkBase(testbase.KcovTestCase):
    def doTest(self, binary):
        self.setUp()
        noKcovRv,o = self.do(testbase.testbuild + "/" + binary, False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/" + binary, False)
        assert rv == noKcovRv

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/" + binary + "/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "fork.c", 21) == 0
        assert parse_cobertura.hitsPerLine(dom, "fork.c", 26) >= 1
        assert parse_cobertura.hitsPerLine(dom, "fork.c", 34) >= 1
        assert parse_cobertura.hitsPerLine(dom, "fork.c", 37) >= 1
        assert parse_cobertura.hitsPerLine(dom, "fork.c", 46) >= 1

class fork_64(ForkBase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only")
    def runTest(self):
        self.doTest("fork")

class fork_32(ForkBase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only")
    def runTest(self):
        self.doTest("fork-32")

class vfork(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/vfork", False)
        assert rv == 0

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/vfork/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "vfork.c", 12) >= 1
        assert parse_cobertura.hitsPerLine(dom, "vfork.c", 18) >= 1

class shared_library(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only, Issue #157")
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.testbuild + "/shared_library_test", False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/shared_library_test", False)
        assert rv == noKcovRv

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.c", 9) >= 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 5) == 1

class shared_library_skip(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only, Issue #157")
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " --skip-solibs " + testbase.outbase + "/kcov " + testbase.testbuild + "/shared_library_test", False)
        assert rv == 0

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 5) == None


class shared_library_filter_out(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only, Issue #157")
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " --exclude-pattern=solib " + testbase.outbase + "/kcov " + testbase.testbuild + "/shared_library_test", False)
        assert rv == 0

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 5) == None


class shared_library_accumulate(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only, Issue #157")
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/shared_library_test 5", False)
        assert rv == 0

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shared_library_test/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.c", 9) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 5) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 10) == 1

class MainTestBase(testbase.KcovTestCase):
    def doTest(self, verify):
        self.setUp()

        noKcovRv,o = self.do(testbase.testbuild + "/main-tests", False)
        rv,o = self.do(testbase.kcov + " " + verify + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-tests 5", False)
        assert rv == noKcovRv

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-tests/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 9) == 1
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 14) == None
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 18) >= 1
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 25) == 1
        assert parse_cobertura.hitsPerLine(dom, "file.c", 6) >= 1
        assert parse_cobertura.hitsPerLine(dom, "file2.c", 7) == 0


class main_test(MainTestBase):
    def runTest(self):
        self.doTest("")

class main_test_verify(MainTestBase):
    def runTest(self):
        self.doTest("--verify")

class main_test_lldb_raw_breakpoints(MainTestBase):
    def runTest(self):
        self.doTest("--configure=lldb-use-raw-breakpoint-writes=1")

class popen_test(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.testbuild + "/test_popen", False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/test_popen", False)
        assert rv == noKcovRv

        assert o.find("popen OK") != -1

class short_filename(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov ./s", False)
        assert rv == 99


class Pie(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.testbuild + "/pie", False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/pie", False)
        assert rv == noKcovRv

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/pie/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "pie.c", 5) == 1

class pie_argv_basic(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/pie-test", False)
        assert rv == 0

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/pie-test/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "argv-dependent.c", 5) == 1
        assert parse_cobertura.hitsPerLine(dom, "argv-dependent.c", 11) == 0

class pie_accumulate(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/pie-test", False)
        assert rv == 0

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/pie-test/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "argv-dependent.c", 5) == 1
        assert parse_cobertura.hitsPerLine(dom, "argv-dependent.c", 11) == 0

        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/pie-test a", False)
        assert rv == 0

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/pie-test/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "argv-dependent.c", 5) == 1
        assert parse_cobertura.hitsPerLine(dom, "argv-dependent.c", 11) == 1


class global_ctors(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.testbuild + "/global-constructors", False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/global-constructors", False)
        assert rv == noKcovRv

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/global-constructors/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "test-global-ctors.cc", 4) >= 1

class daemon_wait_for_last_child(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only, Issue #158")
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.testbuild + "/test_daemon", False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/test_daemon", False)
        assert rv == 4
        assert noKcovRv == 2

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/test_daemon/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "test-daemon.cc", 31) == 1

class SignalsBase(testbase.KcovTestCase):
    def SignalsBase():
        self.m_self = "";

    def cmpOne(self, sig):
        noKcovRv,o = self.do(testbase.testbuild + "/signals " + sig + " " + self.m_self, False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/signals " + sig + " " + self.m_self, False)
        assert rv == noKcovRv

        return parse_cobertura.parseFile(testbase.outbase + "/kcov/signals/cobertura.xml")

    def doTest(self):
        self.setUp()

        dom = self.cmpOne("hup")
        assert parse_cobertura.hitsPerLine(dom, "test-signals.c", 14) == 1

        dom = self.cmpOne("int")
        assert parse_cobertura.hitsPerLine(dom, "test-signals.c", 19) == 1

        dom = self.cmpOne("quit")
        assert parse_cobertura.hitsPerLine(dom, "test-signals.c", 24) == 1

        dom = self.cmpOne("bus")
        assert parse_cobertura.hitsPerLine(dom, "test-signals.c", 44) == 1

        dom = self.cmpOne("fpe")
        assert parse_cobertura.hitsPerLine(dom, "test-signals.c", 49) == 1

        dom = self.cmpOne("term")
        assert parse_cobertura.hitsPerLine(dom, "test-signals.c", 84) == 1

class signals(SignalsBase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only, Issue #158")

    def runTest(self):
        self.m_self = ""
        self.doTest()

class signals_self(SignalsBase):

    def runTest(self):
        self.m_self = "self"
        self.doTest()

class signals_crash(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/signals segv self", False)
        assert o.find("kcov: Process exited with signal 11") != -1

        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/signals abrt self", False)
        assert o.find("kcov: Process exited with signal 6") != -1

class collect_and_report_only(testbase.KcovTestCase):
    # Cannot work with combined Engine / Parser
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only")
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.testbuild + "/main-tests ", False)
        rv,o = self.do(testbase.kcov + " --collect-only " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-tests", False)
        assert rv == noKcovRv

        try:
            dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-tests/cobertura.xml")
            self.fail("File unexpectedly found")
        except:
            # Exception is expected here
            pass

        rv,o = self.do(testbase.kcov + " --report-only " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-tests", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-tests/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 9) == 1

class setpgid_kill(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.sources + "/tests/setpgid-kill/test-script.sh " + testbase.testbuild + "/setpgid-kill", False)
        assert o.find("SUCCESS") != -1
        rv,o = self.do(testbase.sources + "/tests/setpgid-kill/test-script.sh " + testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/setpgid-kill", False)
        assert o.find("SUCCESS") != -1

class attach_process_with_threads(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only")
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.sources + "/tests/daemon/test-script.sh " + testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/issue31", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/issue31/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "test-issue31.cc", 28) >= 1
        assert parse_cobertura.hitsPerLine(dom, "test-issue31.cc", 11) >= 1
        assert parse_cobertura.hitsPerLine(dom, "test-issue31.cc", 8) == 0

class attach_process_with_threads_creates_threads(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only")
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.sources + "/tests/daemon/test-script.sh " + testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/thread-test", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/thread-test/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "thread-main.c", 19) >= 1
        assert parse_cobertura.hitsPerLine(dom, "thread-main.c", 9) >= 1

class merge_same_file_in_multiple_binaries(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/multi_1", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/multi_1/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main_1.c", 10) == 0
        assert parse_cobertura.hitsPerLine(dom, "file.c", 3) == 0

        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/multi_2", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/kcov-merged/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main_2.c", 9) == 1
        assert parse_cobertura.hitsPerLine(dom, "file.c", 3) == 0
        assert parse_cobertura.hitsPerLine(dom, "file.c", 8) == 0

        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/multi_2 1", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/kcov-merged/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "file.c", 3) == 0
        assert parse_cobertura.hitsPerLine(dom, "file.c", 8) == 1


class debuglink(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only")
    def runTest(self):
        self.setUp()
        os.system("rm -rf %s/.debug" % (testbase.outbase))
        os.system("cp %s/main-tests %s/main-tests-debug-file" % (testbase.testbuild, testbase.testbuild))
        os.system("objcopy --only-keep-debug %s/main-tests-debug-file %s/main-tests-debug-file.debug" % (testbase.testbuild, testbase.testbuild))
        os.system("cp %s/main-tests-debug-file.debug %s/main-tests-debug-file1.debug" % (testbase.testbuild, testbase.testbuild))
        os.system("cp %s/main-tests-debug-file.debug %s/main-tests-debug-file12.debug" % (testbase.testbuild, testbase.testbuild))
        os.system("cp %s/main-tests-debug-file.debug %s/main-tests-debug-file123.debug" % (testbase.testbuild, testbase.testbuild))
        os.system("cp %s/main-tests-debug-file %s/main-tests-debug-file1" % (testbase.testbuild, testbase.testbuild))
        os.system("cp %s/main-tests-debug-file %s/main-tests-debug-file2" % (testbase.testbuild, testbase.testbuild))
        os.system("cp %s/main-tests-debug-file %s/main-tests-debug-file3" % (testbase.testbuild, testbase.testbuild))
        os.system("strip -g %s/main-tests-debug-file" % (testbase.testbuild))
        os.system("strip -g %s/main-tests-debug-file1" % (testbase.testbuild))
        os.system("strip -g %s/main-tests-debug-file2" % (testbase.testbuild))
        os.system("strip -g %s/main-tests-debug-file3" % (testbase.testbuild))
        os.system("objcopy --add-gnu-debuglink=%s/main-tests-debug-file.debug %s/main-tests-debug-file" % (testbase.testbuild, testbase.testbuild))
        os.system("objcopy --add-gnu-debuglink=%s/main-tests-debug-file1.debug %s/main-tests-debug-file1" % (testbase.testbuild, testbase.testbuild))
        os.system("objcopy --add-gnu-debuglink=%s/main-tests-debug-file12.debug %s/main-tests-debug-file2" % (testbase.testbuild, testbase.testbuild))
        os.system("objcopy --add-gnu-debuglink=%s/main-tests-debug-file123.debug %s/main-tests-debug-file3" % (testbase.testbuild, testbase.testbuild))

        noKcovRv,o = self.do(testbase.testbuild + "/main-tests-debug-file", False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-tests-debug-file", False)
        assert rv == noKcovRv

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-tests-debug-file/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        # Check alignment
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-tests-debug-file1", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-tests-debug-file1/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-tests-debug-file2", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-tests-debug-file2/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-tests-debug-file3", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-tests-debug-file3/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        # Look in .debug
        os.system("rm -rf %s/kcov" % (testbase.outbase))
        os.system("mkdir -p %s/.debug" % (testbase.testbuild))
        os.system("mv %s/main-tests-debug-file.debug %s/.debug" % (testbase.testbuild, testbase.testbuild))

        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-tests-debug-file", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-tests-debug-file/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 9) == 1

        os.system("rm -rf %s/kcov" % (testbase.outbase))
        os.system("echo 'abc' >> %s/.debug/main-tests-debug-file.debug" % (testbase.testbuild))

        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-tests-debug-file", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-tests-debug-file/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 9) == None
	# Todo: Look in /usr/lib/debug as well

class collect_no_source(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()

        os.system("cp %s/tests/short-file.c %s/main.cc" % (testbase.sources, testbase.testbuild))
        os.system("gcc -g -o %s/main-collect-only %s/main.cc" % (testbase.testbuild, testbase.testbuild))
        os.system("mv %s/main.cc %s/tmp-main.cc" % (testbase.testbuild, testbase.testbuild))

        rv,o = self.do(testbase.kcov + " --collect-only " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-collect-only", False)

        os.system("mv %s/tmp-main.cc %s/main.cc" % (testbase.testbuild, testbase.testbuild))
        rv,o = self.do(testbase.kcov + " --report-only " + testbase.outbase + "/kcov " + testbase.testbuild + "/main-collect-only")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/main-collect-only/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 1) == 1

class dlopen(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.testbuild + "/dlopen", False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.testbuild + "/dlopen", False)

        assert noKcovRv == rv
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/dlopen/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "dlopen.cc", 11) == 1
        assert parse_cobertura.hitsPerLine(dom, "dlopen.cc", 12) == 0
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 5) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 12) == 0


class dlopen_in_ignored_source_file(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " --exclude-pattern=dlopen.cc " + testbase.outbase + "/kcov " + testbase.testbuild + "/dlopen", False)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/dlopen/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "dlopen-main.cc", 10) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 5) == 1
        assert parse_cobertura.hitsPerLine(dom, "solib.c", 12) == 0

class daemon_no_wait_for_last_child(testbase.KcovTestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"), "Linux-only")
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.testbuild + "/test_daemon", False)
        rv,o = self.do(testbase.kcov + " --output-interval=1 --exit-first-process " + testbase.outbase + "/kcov " + testbase.testbuild + "/test_daemon", False)

        assert noKcovRv == rv
        time.sleep(2)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/test_daemon/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "test-daemon.cc", 31) == 0

        time.sleep(5)
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/test_daemon/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "test-daemon.cc", 31) == 1

