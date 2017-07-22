import testbase
import os
import unittest
import parse_cobertura

class BashBase(testbase.KcovTestCase):
    def doTest(self, args):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/shell-main " + args)

        return parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")

class bash_coverage(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/shell-main")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 3) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 4) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 13) == 0
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 26) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 30) == 5
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 34) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 38) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 5) == 1

        assert parse_cobertura.hitsPerLine(dom, "short-test.sh", 5) == 11
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 128) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 132) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 135) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 136) == 1

        assert o.find("I'm echoing to stderr via some") != -1


# Very limited, will expand once it's working better
class bash_coverage_debug_trap(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " --bash-method=DEBUG " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/shell-main 5")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 3) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 4) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 22) == 1

class bash_short_file(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/short-test.sh")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/short-test.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "short-test.sh", 5) == 11

class bash_heredoc_backslashes(BashBase):
    def runTest(self):
        dom = self.doTest("5")

        assert parse_cobertura.hitsPerLine(dom, "shell-main", 52) == 4
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 53) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 55) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 59) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 61) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 62) == None

class bash_heredoc_special_cases_issue_44(BashBase):
    def runTest(self):
        dom = self.doTest("")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 77) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 83) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 88) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 93) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 109) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 118) == 1

class bash_non_empty_braces(BashBase):
    def runTest(self):
        dom = self.doTest("")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 102) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 103) == 3

class bash_coverage_tricky(BashBase):
    def runTest(self):
        dom = self.doTest("5")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 36) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 44) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 7) == None

class bash_honor_signal(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.sources + "/tests/setpgid-kill/test-script.sh " + testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/trap.sh", False)

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/trap.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "trap.sh", 5) == 1

class bash_accumulate_data(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/unitundertest.sh 1")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/unitundertest.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 6) == 1
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 16) == 0
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/unitundertest.sh 2")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/unitundertest.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 6) == 1
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 16) == 1

class bash_accumulate_changed_data(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        os.system("mkdir -p /tmp/test-kcov")
        os.system("cp " + testbase.sources + "/tests/bash/shell-main /tmp/test-kcov")
        os.system("cp " + testbase.sources + "/tests/bash/other.sh /tmp/test-kcov")
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov /tmp/test-kcov/shell-main 5 9")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 40) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 67) == 2
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 6) == 2
        os.system("echo \"echo 'arne-anka'\" >> /tmp/test-kcov/shell-main")
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov /tmp/test-kcov/shell-main 5")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 40) == 0
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 67) == 0
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 6) == 3

class bash_merge_data_issue_38(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/unitundertest.sh 1")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/unitundertest.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 6) == 1
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 16) == 0
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/unitundertest.sh 2")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/unitundertest.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 6) == 1
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 16) == 1
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/unitundertest.sh all")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/unitundertest.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 36) == 1
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 30) == 1
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 33) == 1
        assert parse_cobertura.hitsPerLine(dom, "unitundertest.sh", 36) == 1

class bash_output_crc(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/first-dir/a.sh")
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/second-dir/a.sh")
        rv,a_lines = self.doShell("ls " + testbase.outbase + "/kcov/a.sh/a.sh.*.json")
        rv,b_lines = self.doShell("ls " + testbase.outbase + "/kcov/a.sh/b.sh.*.json")

        assert len(a_lines.split()) == 2
        assert len(b_lines.split()) == 2

class bash_issue_116_arithmetic_and_heredoc_issue_117_comment_within_string(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/shell-main")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 142) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 149) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 151) == 1
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 152) == 1

class bash_multiline_quotes(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/multiline-alias.sh")

        assert o.find("echo called test_alias") == -1
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/multiline-alias.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "multiline-alias.sh", 6) == 1

class bash_multiline_backslashes(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/multiline-backslash.sh")

        assert o.find("function_name") == -1

class bash_no_executed_lines(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/no-executed-statements.sh")

        assert o.find("echo called test_alias") == -1
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/no-executed-statements.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "no-executed-statements.sh", 4) == 0

class bash_stderr_redirection(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/redirect-stderr.sh")

        assert o.find("kcov") == -1
        rv,o = self.do(testbase.kcov + " --debug-force-bash-stderr " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/redirect-stderr.sh")
        assert o.find("kcov") == -1

class bash_dollar_var_replacement(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/dollar-var-replacements.sh")

        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/dollar-var-replacements.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "dollar-var-replacements.sh", 2) == 1
        assert parse_cobertura.hitsPerLine(dom, "dollar-var-replacements.sh", 4) == 1
        assert parse_cobertura.hitsPerLine(dom, "dollar-var-replacements.sh", 5) == 1

# Issue #152
class bash_done_eof(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/shell-main 5")
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 163) == None
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 169) == None

# Issue #154
class bash_eof_backtick(testbase.KcovTestCase):
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/shell-main")
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/shell-main/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "shell-main", 180) == 1

class bash_subshell(testbase.KcovTestCase):
    def runTest(self):
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/subshell.sh")
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/subshell.sh/cobertura.xml")
        self.assertIsNone(parse_cobertura.hitsPerLine(dom, "subshell.sh", 1))
        self.assertEqual(2, parse_cobertura.hitsPerLine(dom, "subshell.sh", 4))
        self.assertEqual(0, parse_cobertura.hitsPerLine(dom, "subshell.sh", 8))

class bash_handle_all_output(testbase.KcovTestCase):
    def runTest(self):
        script = "handle-all-output.sh"
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " +
            testbase.sources + "/tests/bash/" + script,
            timeout=5.0)
        self.assertEqual(0, rv, "kcov exited unsuccessfully")
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/" + script + "/cobertura.xml")
        self.assertIsNone(parse_cobertura.hitsPerLine(dom, script, 1))
        self.assertEqual(1000, parse_cobertura.hitsPerLine(dom, script, 4))

class bash_exit_status(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()
        noKcovRv,o = self.do(testbase.sources + "/tests/bash/shell-main 5", False)
        rv,o = self.do(testbase.kcov + " " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/shell-main 5")

        assert rv == noKcovRv

# Issue 180
class bash_ignore_uncovered(testbase.KcovTestCase):
    @unittest.expectedFailure
    def runTest(self):
        self.setUp()
        rv,o = self.do(testbase.kcov + " --exclude-region=CUSTOM_RANGE_START:CUSTOM_RANGE_END " + testbase.outbase + "/kcov " + testbase.sources + "/tests/bash/other.sh")
        dom = parse_cobertura.parseFile(testbase.outbase + "/kcov/other.sh/cobertura.xml")
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 22) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 23) == 1

        assert parse_cobertura.hitsPerLine(dom, "other.sh", 26) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 27) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 28) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 29) == 1

        assert parse_cobertura.hitsPerLine(dom, "other.sh", 32) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 34) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 35) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 36) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 37) == 1

        assert parse_cobertura.hitsPerLine(dom, "other.sh", 40) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 42) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 43) == 1

        assert parse_cobertura.hitsPerLine(dom, "other.sh", 47) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 48) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 49) == None
        assert parse_cobertura.hitsPerLine(dom, "other.sh", 51) == 1
