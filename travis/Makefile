#!/usr/bin/make

chroot=/tmp/32-bit-chroot
kcov_deps=libdw-dev libelf-dev elfutils libcurl4-openssl-dev python-pip python3 cmake binutils-dev

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
	cd build && cmake ..
	make -C build
	cd build-tools && cmake ../tools
	make -C build-tools
	sudo make -C build install
	cd build-tests && cmake ../tests
	make -C build-tests

build_gcc: build_reference
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

run-performance: build_gcc
	travis/perf-test.sh build/src/kcov . build >/dev/null
	/usr/bin/time -f "%e" travis/perf-test.sh build/src/kcov . build >/dev/null
	rm -rf /tmp/kcov-perf
	travis/perf-test.sh /tmp/kcov-reference . build >/dev/null
	/usr/bin/time -f "%e" travis/perf-test.sh /tmp/kcov-reference . build >/dev/null

run-tests-gcc: run-performance
	sudo i386 chroot ${chroot} sh -c "cd /tmp/kcov/build-tests && WORKSPACE=/tmp/kcov/ pybot --noncritical not_ready ../tests/robot-framework/kcov-tests.txt"
	cd build-tests && WORKSPACE=`pwd`/../ pybot --noncritical not_ready ../tests/robot-framework/kcov-tests.txt
	build/src/kcov --coveralls-id=$(TRAVIS_JOB_ID) --include-pattern=kcov --exclude-pattern=helper.cc,library.cc,html-data-files.cc /tmp/kcov-kcov build/src/kcov || true

run-tests-clang:
	cd build-tests && WORKSPACE=`pwd`/../ pybot --noncritical not_ready ../tests/robot-framework/kcov-tests.txt

build_clang:
	# Stupid workaround for coverity scans...
	sleep 97

run-tests: build run-tests-${CC}

prepare_linux:
	sudo apt-get update -qq
	sudo apt-get install -y ${kcov_deps} gcc-multilib debootstrap git time
	sudo pip install robotframework

prepare_gcc:
	sudo i386 debootstrap --arch=i386 --components=main,universe  precise $(chroot)
	sudo i386 chroot "${chroot}" apt-get update
	sudo i386 chroot "${chroot}" apt-get install -y build-essential
	sudo i386 chroot "${chroot}" apt-get install -y ${kcov_deps}
	sudo i386 chroot "${chroot}" pip install robotframework
	sudo cp -r "$(shell pwd)" "${chroot}/tmp/"
	sudo i386 chroot "${chroot}" sh -c "mount -t proc procfs /proc"


prepare_clang:

prepare_environment: prepare_linux prepare_${CC}
