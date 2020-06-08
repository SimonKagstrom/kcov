[![Build Status](https://travis-ci.org/SimonKagstrom/kcov.svg?branch=master)](https://travis-ci.org/SimonKagstrom/kcov)
[![Coveralls coverage status](https://img.shields.io/coveralls/SimonKagstrom/kcov.svg)](https://coveralls.io/r/SimonKagstrom/kcov?branch=master)
[![Codecov coverage status](https://codecov.io/gh/SimonKagstrom/kcov/branch/master/graph/badge.svg)](https://codecov.io/gh/SimonKagstrom/kcov)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/2844/badge.svg)](https://scan.coverity.com/projects/2844)
![Docker Build Status](https://img.shields.io/docker/build/kcov/kcov.svg)
![MicroBadger Size](https://img.shields.io/microbadger/image-size/kcov/kcov.svg)
![Docker Pulls](https://img.shields.io/docker/pulls/kcov/kcov.svg)

[![PayPal Donate](https://img.shields.io/badge/paypal-donate-blue.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=simon.kagstrom%40gmail%2ecom&lc=US&item_name=Simon%20Kagstrom&item_number=kcov&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donate_LG%2egif%3aNonHosted) [![Github All Releases](https://img.shields.io/github/downloads/atom/atom/total.svg)](https://github.com/SimonKagstrom/kcov/)

## *kcov*
Kcov is a FreeBSD/Linux/OSX code coverage tester for compiled languages, Python
and Bash.  Kcov was originally a fork of [Bcov](http://bcov.sf.net), but has
since evolved to support a large feature set in addition to that of Bcov.

Kcov, like Bcov, uses DWARF debugging information for compiled programs to
make it possible to collect coverage information without special compiler
switches.

For a video introduction, [look at this presentation from SwedenCPP](https://www.youtube.com/watch?v=1QMHbp5LUKg)

Installing
----------
Refer to the [INSTALL](INSTALL.md) file for build instructions, or use our official Docker images:

* [kcov/kcov](https://hub.docker.com/r/kcov/kcov/) for releases since v31.

_Note:_

_Tags v31-v33 are re-tags of [ragnaroek/kcov](https://hub.docker.com/r/ragnaroek/kcov/) which were not automated builds._

_Tags v34-v35 are non automated builds using the Dockerfile in master with 1 minor [modification](https://github.com/SimonKagstrom/kcov/pull/274#issuecomment-432827976)._

_Tag v36 is a non automated build using the Dockerfile in master._




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

Merging multiple kcov runs
--------------------------
Kcov can also merge the results of multiple earlier runs. To use this mode,
call kcov with `--merge`, an output path and one or more paths to an earlier
run, e.g.,

```
kcov --merge /tmp/merged-output /tmp/kcov-output1 /tmp/kcov-output2
kcov --merge /tmp/merged-output /tmp/kcov-output*    # With a wildcard
```

Use from continuous integration systems
---------------------------------------
kcov is easy to integrate with [travis-ci](http://travis-ci.org) together with
[coveralls.io](http://coveralls.io) or [codecov.io](http://codecov.io). It can also
be used from Jenkins, [SonarQube](http://sonarqube.org) and [GitLab CI](http://gitlab.com).
Refer to

* [coveralls](doc/coveralls.md) for details about travis-ci + coveralls, or
* [codecov](doc/codecov.md) for details about travis-ci + codecov
* [jenkins](doc/jenkins.md) for details about how to integrate in Jenkins
* [sonarqube](doc/sonarqube.md) for how to use kcov and sonarqube together
* [gitlab](doc/gitlab.md) for use with GitLab

Full-system instrumentation
---------------------------
Kcov can instrument all binaries with very low overhead for embedded systems.
Refer to the [full system instrumentation](doc/full-system-instrumentation.md) documentation for details.

More information
----------------
kcov is written by Simon Kagstrom <simon.kagstrom@gmail.com> and more
information can be found at [the web page](http://simonkagstrom.github.com/kcov/index.html)
