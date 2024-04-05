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

default_timeout = 10 * 60


class TestCase(unittest.TestCase):
    def __init__(self, kcov, outbase, binaries, sources):
        super().__init__()

        self.kcov = kcov
        self.kcov_system_daemon = self.kcov + "-system-daemon"
        self.outbase = outbase
        self.outdir = self.outbase + "/" + "kcov"
        self.binaries = binaries
        self.sources = sources

    def setUp(self):
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

    def do(self, cmdline, /, kcovKcov=True, *, timeout=default_timeout):
        extra = ""
        if (
            kcovKcov
            and sys.platform.startswith("linux")
            and platform.machine() in ["x86_64", "i386", "i686"]
        ):
            extra = (
                self.kcov
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
