
## *Building Kcov*

Installing dependencies
=======================
You need development headers and libraries for libstdc++, curl, elfutils
and (optional) binutils and libiberty to build kcov. Note that elfutils is
found in multiple variants, and at least in RH/Centos/Fedora you'll need
elfutils-devel and *not* elfutils-libelf-devel.

FreeBSD
-------

```sh
pkg install binutils cmake elfutils python
```

Alpine
-------

```sh
apk add --update --no-cache curl-dev curl-static libdw openssl-dev zlib-dev elfutils-dev libstdc++-dev
```

Ubuntu / Debian
---------------
Install

```sh
apt-get install binutils-dev libssl-dev libcurl4-openssl-dev libelf-dev libstdc++-12-dev zlib1g-dev libdw-dev libiberty-dev
```

Fedora / Centos / RHEL
----------------------
Install elfutils-libelf-devel libcurl-devel binutils-devel elfutils-devel

Mac OS
--------
Install dependencies:

```sh
brew install zlib bash cmake pkgconfig dwarfutils openssl ninja
```

Mac OS build instructions (see Issue #166 / Issue #357):

Create an empty build dir and do the following steps (adjust openssl path, can also be in /opt):

```sh
  cd <build-dir>
  cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) <path/to/kcov/source/dir>
  make
```

The binary will be `src/Release/kcov`

If the binary needs to be signed, try these instructions:

First get a code signing identity, since debug support is needed to run as non-root:

```sh
codesign -s - --entitlements <path/to/kcov/source/dir>/osx-entitlements.xml -f ./src/Release/kcov
```

Building
========

Create an empty build dir and do the following steps:

```sh
cmake [options] <path/to/kcov/source/dir>
make
make install
```

Useful options include -DCMAKE_INSTALL_PREFIX=<path> (installation prefix),
-DCMAKE_BUILD_TYPE=<type> and -DCMAKE_C_FLAGS=<CFLAGS>.

Basic example:

```sh
    cd /path/to/kcov/source/dir
    mkdir build
    cd build
    cmake ..
    make
    make install
```

More advanced example:

```sh
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

```sh
    cd kcov
    CMAKE_PREFIX_PATH=/opt/elfutils-dir/ \
    cmake .
    make
    make install
```
