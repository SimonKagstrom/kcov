
## *Building Kcov*

Installing dependencies
=======================
You need development headers and libraries for libstdc++, curl, elfutils
and (optional) binutils and libiberty to build kcov. Note that elfutils is
found in multiple variants, and at least in RH/Centos/Fedora you'll need
elfutils-devel and *not* elfutils-libelf-devel.

Ubuntu
------
Install binutils-dev libcurl4-openssl-dev zlib1g-dev libdw-dev libiberty-dev


Fedora / Centos / RHEL
----------------------
Install elfutils-libelf-devel libcurl-devel binutils-devel elfutils-devel

Mac OS X
--------
brew install homebrew/dupes/zlib bash cmake pkgconfig

*Temporary* OSX build instructions due to Issue #166:

Create an empty build dir and do the following steps:

```
  cd <build-dir>
  cmake -G Xcode <path/to/kcov/source/dir>
  xcodebuild -configuration Release
```

The binary will be in `src/Release/kcov`

Building
========

Create an empty build dir and do the following steps:

    cmake [options] <path/to/kcov/source/dir>
    make
    make install

Useful options include -DCMAKE_INSTALL_PREFIX=<path> (installation prefix),
-DCMAKE_BUILD_TYPE=<type> and -DCMAKE_C_FLAGS=<CFLAGS>.

Basic example:

```
    cd /path/to/kcov/source/dir
    mkdir build
    cd build
    cmake ..
    make
    make install
```

More advanced example:

```
    cd /path/to/kcov/source/dir
    mkdir build
    cd build
    cmake \
      -DCMAKE_C_FLAGS:STRING="-O3 -march=i686" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      ..
    make -j2 || exit 1
    make install DESTDIR=/tmp/kcov-build || exit 1
```

For further information refer to cmake documentation:
    http://www.cmake.org/cmake/help/cmake-2-8-docs.html.


Troubleshooting
===============

If you have elfutils installed, but cmake fails to find it, specify elfutils
install prefix explicitly to cmake. Here is an example:

```
    cd kcov
    CMAKE_PREFIX_PATH=/opt/elfutils-dir/ \
    cmake .
    make
    make install
```
