[![Build Status](https://travis-ci.org/SimonKagstrom/kcov.svg?branch=master)](https://travis-ci.org/SimonKagstrom/kcov) [![Coverage Status](https://img.shields.io/coveralls/SimonKagstrom/kcov.svg)](https://coveralls.io/r/SimonKagstrom/kcov?branch=master)

Kcov is a code coverage tester based on Bcov (http://bcov.sf.net).

Kcov, like Bcov, uses DWARF debugging information to make it possible
to collect coverage information without special compiler switches.

Simple usage:

   kcov /path/to/outdir executable [args for the executable]

/path/to/outdir will contain lcov-style HTML output generated
continously while the application runs.

kcov is written by Simon Kagstrom <simon.kagstrom@gmail.com> and more
information can be found at the web page,

  http://simonkagstrom.github.com/kcov/index.html
