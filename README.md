[![Build Status](https://travis-ci.org/SimonKagstrom/kcov.svg?branch=master)](https://travis-ci.org/SimonKagstrom/kcov) [![Coverage Status](https://img.shields.io/coveralls/SimonKagstrom/kcov.svg)](https://coveralls.io/r/SimonKagstrom/kcov?branch=master)

## *kcov*

What is it?
-----------
Kcov is a code coverage tester for compiled languages, Python and Bash.
Kcov was originally a fork of [Bcov](http://bcov.sf.net), but has since
evolved to support a large feature set in addition to that of Bcov.

Kcov, like Bcov, uses DWARF debugging information for compiled programs to
make it possible to collect coverage information without special compiler
switches.

How to use it
-------------
Basic usage is straight-forward:

```
kcov /path/to/outdir executable [args for the executable]
```

*/path/to/outdir* will contain lcov-style HTML output generated
continuously while the application runs. Kcov will also write cobertura-
compatible XML output and can upload coverage data directly to
http://coveralls.io for easy integration with travis-ci.

Filtering output
----------------
It's often useful to filter output, since e.g., /usr/include is seldomly of interest.
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


More information
----------------
kcov is written by Simon Kagstrom <simon.kagstrom@gmail.com> and more
information can be found at [the web page](http://simonkagstrom.github.com/kcov/index.html)
