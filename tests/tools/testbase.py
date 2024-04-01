#!/usr/bin/env python3

import errno
import os
import os.path
import platform
import shutil
import subprocess
import sys
import time
import unittest

PIPE = subprocess.PIPE

kcov = ""
outbase = ""
testbuild = ""
sources = ""

default_timeout = 10 * 60


# Normalize path, also ensuring that it is not empty.
def normalize(path):
    assert len(path) != 0, "path must be not empty"

    path = os.path.normpath(path)
    return path


def configure(k, o, t, s):
    global kcov, outbase, testbuild, sources, kcov_system_daemon

    kcov = normalize(k)
    outbase = normalize(o)
    testbuild = normalize(t)
    sources = normalize(s)

    assert os.path.abspath(outbase) != os.getcwd(), "'outbase' cannot be the current directory"


class KcovTestCase(unittest.TestCase):
    def setUp(self):
        # Make the configuration values available as class members.
        self.kcov = kcov
        self.kcov_system_daemon = self.kcov + "-system-daemon"
        self.outbase = outbase
        self.outdir = self.outbase + "/" + "kcov"
        self.testbuild = testbuild
        self.sources = sources

        # Intentionally fails if target directory exists.
        os.makedirs(self.outdir)

    def tearDown(self):
        # Don't ignore errors, since they may be caused by bugs in the test suite.
        try:
            shutil.rmtree(self.outdir)
        except OSError as err:
            if err.errno != errno.ENOTEMPTY:
                raise

            # See https://github.com/ansible/ansible/issues/34335 as an example.
            # Sleeping is necessary to ensure no more files are created.
            time.sleep(5)
            sys.stderr.write(f"warning: retry rmtree: {err}\n")
            shutil.rmtree(self.outdir)

    def doShell(self, cmdline):
        term = subprocess.run(
            cmdline, shell=True, stdout=PIPE, stderr=PIPE, timeout=default_timeout
        )
        output = term.stdout + term.stderr
        rv = term.returncode

        return rv, output

    def do(self, cmdline, kcovKcov=True, timeout=default_timeout):
        extra = ""
        if (
            kcovKcov
            and sys.platform.startswith("linux")
            and platform.machine() in ["x86_64", "i386", "i686"]
        ):
            extra = (
                kcov
                + " --include-pattern=kcov --exclude-pattern=helper.cc,library.cc,html-data-files.cc "
                + "/tmp/kcov-kcov "
            )

        cmdline = extra + cmdline
        args = cmdline.split()
        term = subprocess.run(args, stdout=PIPE, stderr=PIPE, timeout=timeout)
        output = term.stdout + term.stderr
        rv = term.returncode

        return rv, output

    def doCmd(self, cmdline):
        args = cmdline.split()
        term = subprocess.run(args, stdout=PIPE, stderr=PIPE, timeout=default_timeout)
        output = term.stdout + term.stderr
        rv = term.returncode

        return rv, output

    def write_message(self, msg):
        """Add a custom message to the current test status line."""

        sys.stderr.write(msg + " ")
