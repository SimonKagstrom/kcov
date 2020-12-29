#!/usr/bin/env python3

import unittest
import os
import sys
import subprocess
import time
import threading

kcov = ""
kcov_system_daemon = ""
outbase = ""
testbuild = ""
sources = ""

def configure(k, o, t, s):
    global kcov, outbase, testbuild, sources, kcov_system_daemon
    kcov = k
    kcov_system_daemon = k + "-system-daemon"
    outbase = o
    testbuild = t
    sources = s

class KcovTestCase(unittest.TestCase):
    def setUp(self):
        if outbase != "":
            os.system("/bin/rm -rf %s/kcov" % (outbase))
        os.system("/bin/mkdir -p %s/kcov/" % (outbase))

    def doShell(self, cmdline):
        child = subprocess.Popen(cmdline, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = child.communicate()
        output = out + err
        rv = child.returncode

        return rv, output

    def do(self, cmdline, kcovKcov = True, timeout = None, kill = False):
        output = ""
        rv = 0

        extra = ""
        if kcovKcov and sys.platform.startswith("linux"):
            extra = kcov + " --include-pattern=kcov --exclude-pattern=helper.cc,library.cc,html-data-files.cc " + outbase + "/kcov-kcov "


        cmdline = extra + cmdline
        child = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        timer = None

        if timeout is not None:
            def stopChild():
                print("\n  didn't finish within %s seconds; killing ..." % timeout)
                if kill:
                    child.kill()
                else:
                    child.terminate()

            timer = threading.Timer(timeout, stopChild)
            timer.start()

        out, err = child.communicate()
        if timer is not None:
            timer.cancel()
        output = out + err
        rv = child.returncode

        return rv, output
