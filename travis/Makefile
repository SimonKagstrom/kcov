#!/usr/bin/make

chroot=/tmp/32-bit-chroot
.PHONY: prepare_environment

build:
	mkdir -p build
	cd build && cmake ..
	make -C build
	
build-tests: build
	mkdir -p build-tests
	cd build-tests && cmake ../tests
	make -C build-tests
	
run-tests: build-tests
	cd build-tests && WORKSPACE=`pwd`/../ pybot --noncritical not_ready ../tests/robot-framework/kcov-tests.txt

prepare_environment:
	sudo apt-get update -qq
	sudo apt-get install -y libdw-dev libelf-dev elfutils libcurl4-openssl-dev python-pip python3 gcc-multilib debootstrap
	sudo pip install robotframework
	sudo mkdir -p "${chroot}/$(shell pwd)"
	sudo i386 debootstrap --arch=i386 precise $(chroot)
	sudo mount --rbind "$(shell pwd)" "${chroot}/$(shell pwd)"
	sudo i386 chroot "${chroot}" apt-get update
	sudo i386 chroot "${chroot}" apt-get install -y build-essential
	sudo i386 chroot "${chroot}" apt-get install -y libdw-dev libelf-dev elfutils libcurl4-openssl-dev python-pip python3
