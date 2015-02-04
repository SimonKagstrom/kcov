[![Build Status](https://travis-ci.org/SimonKagstrom/kcov.svg?branch=master)](https://travis-ci.org/SimonKagstrom/kcov) [![Coverage Status](https://img.shields.io/coveralls/SimonKagstrom/kcov.svg)](https://coveralls.io/r/SimonKagstrom/kcov?branch=master)

Kcov is a code coverage tester for compiled languages, Python and Bash.
Kcov was originally a fork of Bcov (http://bcov.sf.net), but has since
evolved to support a large feature set in addition to that of Bcov.

Kcov, like Bcov, uses DWARF debugging information for compiled programs to
make it possible to collect coverage information without special compiler
switches.

The usage is simple:

   kcov /path/to/outdir executable [args for the executable]

/path/to/outdir will contain lcov-style HTML output generated
continuously while the application runs. Kcov will also write cobertura-
compatible XML output and can upload coverage data directly to
http://coveralls.io for easy integration with travis-ci.

kcov is written by Simon Kagstrom <simon.kagstrom@gmail.com> and more
information can be found at the web page,

  http://simonkagstrom.github.com/kcov/index.html
