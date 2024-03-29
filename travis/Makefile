#!/usr/bin/make

chroot=/tmp/32-bit-chroot
kcov_deps=libdw-dev libelf-dev elfutils libcurl4-openssl-dev python python3 cmake binutils-dev git

.PHONY: prepare_environment

build_reference:
	mkdir -p reference/src reference/build
	cd reference/ && git clone https://github.com/SimonKagstrom/kcov.git
	cd reference/kcov && git checkout v23
	cd reference/build && cmake ../kcov
	make -C reference/build/
	cp reference/build/src/kcov /tmp/kcov-reference

build:
	mkdir -p build build-tests build-tools
	cd build && CXXFLAGS=-Werror cmake ..
	make -C build
	cd build-tools && cmake ../tools
	make -C build-tools
	sudo make -C build install
	cd build-tests && cmake ../tests
	make -C build-tests

build_gcc: build run_unit_tests build_reference patchelf-i386-chroot
	sudo rm -rf ${chroot}/tmp/kcov/build ${chroot}/tmp/kcov/build-tests
	sudo mkdir -p ${chroot}/tmp/kcov/build ${chroot}/tmp/kcov/build-tests ${chroot}/tmp/kcov/build-tools
	sudo i386 chroot ${chroot} sh -c "cd /tmp/kcov/build && cmake .."
	sudo i386 chroot ${chroot} sh -c "make -C /tmp/kcov/build"
	sudo i386 chroot ${chroot} sh -c "make -C /tmp/kcov/build install"
	sudo i386 chroot ${chroot} sh -c "cd /tmp/kcov/build-tools && cmake ../tools"
	sudo i386 chroot ${chroot} sh -c "make -C /tmp/kcov/build-tools"
	sudo i386 chroot ${chroot} sh -c "cd /tmp/kcov/build-tests && cmake ../tests"
	sudo i386 chroot ${chroot} sh -c "make -C /tmp/kcov/build-tests"
	sudo i386 chroot ${chroot} sh -c "apt-get -y remove binutils-dev"
	sudo i386 chroot ${chroot} sh -c "mkdir -p /tmp/kcov/build-no-binutils && cd /tmp/kcov/build-no-binutils && cmake .."
	sudo i386 chroot ${chroot} sh -c "make -C /tmp/kcov/build-no-binutils"

build_clang_linux:
	mkdir -p build build-tests build-tools
	cd build && CC=clang-3.9 CXX=clang++-3.9 CXXFLAGS=-Werror cmake ..
	make -C build
	cd build-tools && cmake ../tools
	make -C build-tools
	sudo make -C build install
	cd build-tests && CC=clang-3.9 CXX=clang++-3.9 cmake ../tests
	CC=clang-3.9 CXX=clang++-3.9 make -C build-tests

build_clang:
	mkdir -p build build-tests build-tools
	cd build && CC=clang-3.9 CXX=clang++-3.9 CXXFLAGS=-Werror cmake ..
	make -C build
	cd build-tools && cmake ../tools
	make -C build-tools
	sudo make -C build install
	cd build-tests && CC=clang-3.9 CXX=clang++-3.9 cmake ../tests
	CC=clang-3.9 CXX=clang++-3.9 make -C build-tests

run-performance: build_gcc
	travis/perf-test.sh build/src/kcov . build >/dev/null
	/usr/bin/time -f "%e" travis/perf-test.sh build/src/kcov . build >/dev/null
	rm -rf /tmp/kcov-perf
	travis/perf-test.sh /tmp/kcov-reference . build >/dev/null
	/usr/bin/time -f "%e" travis/perf-test.sh /tmp/kcov-reference . build >/dev/null

run-tests-linux-gcc: run-performance
	tests/tools/run-tests build/src/kcov /tmp/ build-tests/ `pwd` -v
	sudo i386 chroot ${chroot} sh -c "mkdir -p /tmp/output"
	sudo i386 chroot ${chroot} sh -c "PATH=/bin:/usr/bin:/sbin:/usr/sbin /tmp/kcov/tests/tools/run-tests /tmp/kcov/build/src/kcov /tmp/output /tmp/kcov/build-tests/ /tmp/kcov -v"
	build/src/kcov --coveralls-id=$(TRAVIS_JOB_ID) --include-pattern=kcov --exclude-pattern=helper.cc,library.cc,html-data-files.cc /tmp/kcov-kcov build/src/kcov || true

run-tests-linux-clang: build_clang_linux
	tests/tools/run-tests build/src/kcov /tmp/ build-tests/ `pwd` -v

trompeloeil:
	git submodule update --recursive --remote

patchelf:
	wget https://github.com/NixOS/patchelf/archive/0.9.tar.gz
	tar -xf 0.9.tar.gz
	cd patchelf-0.9 && ./bootstrap.sh && ./configure && make && sudo make install

patchelf-i386-chroot: patchelf
	wget https://github.com/NixOS/patchelf/archive/0.9.tar.gz
	sudo cp 0.9.tar.gz ${chroot}
	sudo i386 chroot ${chroot} sh -c "tar -xf 0.9.tar.gz"
	sudo i386 chroot ${chroot} sh -c "cd patchelf-0.9 && ./bootstrap.sh && ./configure && make && make install"

build_unit_tests: trompeloeil
	mkdir -p build-ut
	cd build-ut && CXX=g++-6 CC=gcc-6 cmake ../tests/unit-tests/
	CXX=g++-6 CC=gcc-6 make -C build-ut

run_unit_tests: build build_unit_tests
	cd build-ut && ./ut

run-tests: run-tests-${TRAVIS_OS_NAME}-${CC}

prepare_linux: patchelf
	sudo sh -c "echo 0 > /proc/sys/kernel/yama/ptrace_scope"
	sudo add-apt-repository ppa:jonlar/crpcut -y
	sudo apt-get update -q
	sudo apt-get install -y --allow-unauthenticated ${kcov_deps} gcc-multilib debootstrap git time curl crpcut crpcut-dev

prepare_linux_gcc:
	sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
	sudo apt-get update -q
	sudo apt-get install g++-6

	sudo i386 debootstrap --arch=i386 --components=main,universe  precise $(chroot)
	sudo i386 chroot "${chroot}" apt-get update
	sudo i386 chroot "${chroot}" apt-get install -y build-essential autoconf
	sudo i386 chroot "${chroot}" apt-get install -y --allow-unauthenticated ${kcov_deps}
	sudo cp -r "$(shell pwd)" "${chroot}/tmp/"
	sudo i386 chroot "${chroot}" sh -c "mount -t proc procfs /proc"
	sudo i386 chroot "${chroot}" sh -c "mount -t tmpfs none /dev/shm"


prepare_linux_clang:
	sudo apt-add-repository -y 'deb http://llvm.org/apt/trusty llvm-toolchain-trusty-3.9 main'
	sudo apt-get update --force-yes -qq
	sudo apt-get install -qq --force-yes -y clang-3.9 libboost-all-dev binutils-dev

prepare_environment: prepare_${TRAVIS_OS_NAME} prepare_${TRAVIS_OS_NAME}_${CC}
