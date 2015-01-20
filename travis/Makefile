#!/usr/bin/make

chroot=/tmp/32-bit-chroot
kcov_deps=libdw-dev libelf-dev elfutils libcurl4-openssl-dev python-pip python3 cmake

.PHONY: prepare_environment

build_reference:
	mkdir -p build-reference
	git checkout 3b9473f4731d0c911cc6934c86ce9e1db291c077 && cd build-reference && cmake ..
	make -C build-reference
	cp build-reference/src/kcov /tmp/kcov-reference
	git checkout master

build: build_reference
	mkdir -p build build-tests
	cd build && cmake ..
	make -C build
	sudo make -C build install
	cd build-tests && cmake ../tests
	make -C build-tests

build_gcc:
	sudo rm -rf ${chroot}/tmp/kcov/build ${chroot}/tmp/kcov/build-tests
	sudo mkdir -p ${chroot}/tmp/kcov/build ${chroot}/tmp/kcov/build-tests
	sudo i386 chroot ${chroot} sh -c "cd /tmp/kcov/build && cmake .."
	sudo i386 chroot ${chroot} sh -c "make -C /tmp/kcov/build"
	sudo i386 chroot ${chroot} sh -c "make -C /tmp/kcov/build install"
	sudo i386 chroot ${chroot} sh -c "cd /tmp/kcov/build-tests && cmake ../tests"
	sudo i386 chroot ${chroot} sh -c "make -C /tmp/kcov/build-tests"

run-tests-gcc: build_gcc
	sudo i386 chroot ${chroot} sh -c "cd /tmp/kcov/build-tests && WORKSPACE=/tmp/kcov/ pybot --noncritical not_ready ../tests/robot-framework/kcov-tests.txt"

run-tests-clang:

build_clang:

run-tests: build run-tests-${CC}
	cd build-tests && WORKSPACE=`pwd`/../ pybot --noncritical not_ready ../tests/robot-framework/kcov-tests.txt

prepare_linux:
	sudo apt-get update -qq
	sudo apt-get install -y ${kcov_deps} gcc-multilib debootstrap
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
