[![Build Status](https://travis-ci.org/SimonKagstrom/kcov.svg?branch=master)](https://travis-ci.org/SimonKagstrom/kcov) [![Coveralls coverage status](https://img.shields.io/coveralls/SimonKagstrom/kcov.svg)](https://coveralls.io/r/SimonKagstrom/kcov?branch=master) [![Codecov coverage status](https://codecov.io/gh/SimonKagstrom/kcov/branch/master/graph/badge.svg)](https://codecov.io/gh/SimonKagstrom/kcov) [![Coverity Scan Build Status](https://scan.coverity.com/projects/2844/badge.svg)](https://scan.coverity.com/projects/2844)

[![PayPal Donate](https://img.shields.io/badge/paypal-donate-blue.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=simon.kagstrom%40gmail%2ecom&lc=US&item_name=Simon%20Kagstrom&item_number=kcov&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donate_LG%2egif%3aNonHosted) [![Github All Releases](https://img.shields.io/github/downloads/atom/atom/total.svg)](https://github.com/SimonKagstrom/kcov/)

## *kcov*
Kcov is a Linux/OSX code coverage tester for compiled languages, Python and Bash.
Kcov was originally a fork of [Bcov](http://bcov.sf.net), but has since
evolved to support a large feature set in addition to that of Bcov.

Kcov, like Bcov, uses DWARF debugging information for compiled programs to
make it possible to collect coverage information without special compiler
switches.

Installing
----------
Refer to the [INSTALL](INSTALL.md) file for build instructions, or use one of the pre-built Docker images:

* [ragnaroek/kcov](https://hub.docker.com/r/ragnaroek/kcov/) for releases since v31
* [ragnaroek/kcov_head](https://hub.docker.com/r/ragnaroek/kcov_head/) for the latest git HEAD (might be unstable)

How to use it
-------------
Basic usage is straight-forward:

```
kcov /path/to/outdir executable [args for the executable]
```

*/path/to/outdir* will contain lcov-style HTML output generated
continuously while the application runs. Kcov will also write cobertura-
compatible XML output and can upload coverage data directly to
http://coveralls.io for easy integration with travis-ci. A generic
coverage.json report is also generated which contains summaries for a given
binary and each source file.

Filtering output
----------------
It's often useful to filter output, since e.g., /usr/include is seldom of interest.
This can be done in two ways:

```
kcov --exclude-pattern=/usr/include --include-pattern=part/of/path,other/path \
      /path/to/outdir executable
```

which will do a string-comparison and include everything which contains
*part/of/path* or *other/path* but exclude everything that has the
*/usr/include* string in it.

```
kcov --include-path=/my/src/path /path/to/outdir executable
kcov --exclude-path=/usr/include /path/to/outdir executable
```

Does the same thing, but with proper path lookups.

Travis-ci / coveralls integration
---------------------------------
kcov coverage collection is easy to integrate with [travis-ci](http://travis-ci.org) and
[coveralls.io](http://coveralls.io). To upload data from the travis build to coveralls,
run kcov with

```
kcov --coveralls-id=$TRAVIS_JOB_ID /path/to/outdir executable
```

which in addition to regular coverage collection uploads to coveralls.

Travis-ci / codecov integration
---------------------------------
Integrating with [codecov](http://codecov.io) is also simple. To upload data from the travis build to codecov, run kcov normally, and then upload using the codecov uploader

```
kcov /path/to/outdir executable
bash <(curl -s https://codecov.io/bash) -s /path/to/outdir
```

The easiest way to achieve this is to run the codecov uploader on travis success:

```
after_success:
  - bash <(curl -s https://codecov.io/bash) -s /path/to/outdir
```

the -s /path/to/outdir part can be skipped if kcov produces output in the current directory.

Full-system instrumentation
---------------------------
Using [dyninst](http://www.dyninst.org), Kcov can instrument all binaries with very low overhead for embedded systems.
Refer to the [full system instrumentation](doc/full-system-instrumentation.md) documentation for details.

More information
----------------
kcov is written by Simon Kagstrom <simon.kagstrom@gmail.com> and more
information can be found at [the web page](http://simonkagstrom.github.com/kcov/index.html)
