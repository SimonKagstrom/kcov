#!/usr/bin/env python

import unittest
import os
import sys
import subprocess
import time

kcov = ""
outbase = ""
testbuild = ""
sources = ""

def configure(k, o, t, s):
    global kcov, outbase, testbuild, sources
    kcov = k
    outbase = o
    testbuild = t
    sources = s

class KcovTestCase(unittest.TestCase):
    def setUp(self):
        if outbase != "":
            os.system("rm -rf %s/kcov" % (outbase))
        os.system("mkdir -p %s/kcov/" % (outbase))

    def doShell(self, cmdline):
        child = subprocess.Popen(cmdline, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = child.communicate()
        output = out + err
        rv = child.returncode

        return rv, output

    def do(self, cmdline, kcovKcov = True):
        output = ""
        rv = 0

        extra = ""
        if not sys.platform.startswith("linux"):
            kcovKcov = True

        if kcovKcov:
            extra = kcov + " --include-pattern=kcov --exclude-pattern=helper.cc,library.cc,html-data-files.cc " + outbase + "/kcov-kcov "


        cmdline = extra + cmdline
        child = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = child.communicate()
        output = out + err
        rv = child.returncode

        return rv, output
