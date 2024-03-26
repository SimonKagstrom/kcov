#!/usr/bin/env python3

import os
import os.path
import platform
import shutil
import subprocess
import sys
import threading
import unittest

kcov = ""
kcov_system_daemon = ""
outbase = ""
testbuild = ""
sources = ""


# Normalize path, also ensuring that it is not empty.
def normalize(path):
    assert len(path) != 0, "path must be not empty"

    path = os.path.normpath(path)
    return path


def configure(k, o, t, s):
    global kcov, outbase, testbuild, sources, kcov_system_daemon

    kcov = normalize(k)
    kcov_system_daemon = k + "-system-daemon"
    outbase = normalize(o)
    testbuild = normalize(t)
    sources = normalize(s)

    assert os.path.abspath(outbase) != os.getcwd(), "'outbase' cannot be the current directory"


class KcovTestCase(unittest.TestCase):
    def setUp(self):
        os.makedirs(outbase + "/" + "kcov")

    def tearDown(self):
        shutil.rmtree(outbase + "/" + "kcov")

    def doShell(self, cmdline):
        child = subprocess.Popen(
            cmdline, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        out, err = child.communicate()
        output = out + err
        rv = child.returncode

        return rv, output

    def do(self, cmdline, kcovKcov=True, timeout=None, kill=False):
        output = ""
        rv = 0

        extra = ""
        if (
            kcovKcov
            and sys.platform.startswith("linux")
            and platform.machine() in ["x86_64", "i386", "i686"]
        ):
            extra = (
                kcov
                + " --include-pattern=kcov --exclude-pattern=helper.cc,library.cc,html-data-files.cc "
                + outbase
                + "/kcov-kcov "
            )

        cmdline = extra + cmdline
        child = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        timer = None

        if timeout is not None:

            def stopChild():
                print(f"\n  didn't finish within {timeout} seconds; killing ...")
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
