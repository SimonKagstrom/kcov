#!/usr/bin/make

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
	sudo apt-get install -y libdw-dev libelf-dev elfutils libcurl4-openssl-dev python-pip python3 gcc-multilib
	sudo pip install robotframework
