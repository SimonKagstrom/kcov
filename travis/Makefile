#!/usr/bin/make

chroot=/tmp/32-bit-chroot
kcov_deps=libdw-dev libelf-dev elfutils libcurl4-openssl-dev python-pip python3

.PHONY: prepare_environment

build:
	mkdir -p build build-tests
	cd build && cmake ..
	make -C build
	cd build-tests && cmake ../tests
	make -C build-tests

build_gcc:
	sudo mkdir -p ${chroot}/$(shell pwd)/build-32 ${chroot}/$(shell pwd)/build-32-tests
	sudo i386 chroot ${chroot} && cd $(shell pwd)/build-32 && cmake ..
	sudo i386 chroot ${chroot} && make -C $(shell pwd)/build-32
	sudo i386 chroot ${chroot} && cd $(shell pwd)/build-tests-32 && cmake ../tests
	sudo i386 chroot ${chroot} && make -C $(shell pwd)/build-tests-32

build_clang:

run-tests: build build_${CC}
	cd build-tests && WORKSPACE=`pwd`/../ pybot --noncritical not_ready ../tests/robot-framework/kcov-tests.txt

prepare_environment:
	sudo apt-get update -qq
	sudo apt-get install -y ${kcov_deps} gcc-multilib debootstrap
	sudo pip install robotframework
	sudo mkdir -p "${chroot}/$(shell pwd)"
	sudo i386 debootstrap --arch=i386 --components=main,universe  precise $(chroot)
	sudo mount --rbind "$(shell pwd)" "${chroot}/$(shell pwd)"
	sudo i386 chroot "${chroot}" apt-get update
	sudo i386 chroot "${chroot}" apt-get install -y build-essential
	sudo i386 chroot "${chroot}" apt-get install -y ${kcov_deps}
