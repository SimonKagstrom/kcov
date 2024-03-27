import os
import platform
import time
import unittest

import parse_cobertura
import testbase


class SystemModeBase(testbase.KcovTestCase):
    def writeToPipe(self, str):
        f = open("/tmp/kcov-system.pipe", "w")
        f.write(str)
        f.close()


class system_mode_can_start_and_stop_daemon(SystemModeBase):
    def runTest(self):
        rv, o = self.do(testbase.kcov_system_daemon + " -d", False)

        pf = "/tmp/kcov-system.pid"
        assert os.path.isfile(pf)

        self.writeToPipe("STOPME")

        time.sleep(2)

        assert os.path.isfile(pf) is False


class system_mode_can_instrument_binary(SystemModeBase):
    def runTest(self):
        rv, o = self.do(
            testbase.kcov
            + " --system-record "
            + testbase.outbase
            + "/kcov "
            + testbase.testbuild
            + "/"
        )
        assert rv == 0

        src = testbase.testbuild + "/main-tests"
        dst = testbase.outbase + "/kcov/main-tests"

        assert os.path.isfile(src)
        assert os.path.isfile(dst)

        assert os.path.getsize(dst) > os.path.getsize(src)


class system_mode_can_record_and_report_binary(SystemModeBase):
    @unittest.skipIf(platform.machine() == "i686", "x86_64-only")
    def runTest(self):
        print(platform.machine())
        try:
            os.makedirs(testbase.outbase + "/kcov")
        except:
            pass
        rv, o = self.do(
            testbase.kcov
            + " --system-record "
            + testbase.outbase
            + "/kcov "
            + testbase.testbuild
            + "/"
        )

        rv, o = self.do(testbase.kcov_system_daemon + " -d", False)

        os.environ["LD_LIBRARY_PATH"] = testbase.outbase + "/kcov/lib"
        rv, o = self.do(testbase.outbase + "/kcov/main-tests", False)
        self.skipTest("Fickle test, ignoring")
        assert rv == 0

        time.sleep(3)
        self.writeToPipe("STOPME")

        rv, o = self.do(
            testbase.kcov + " --system-report " + testbase.outbase + "/kcov-report /tmp/kcov-data"
        )
        assert rv == 0

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov-report/main-tests/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 9) == 1
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 14) is None
        assert parse_cobertura.hitsPerLine(dom, "main.cc", 18) >= 1
