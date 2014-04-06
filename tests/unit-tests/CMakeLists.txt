cmake_minimum_required (VERSION 2.6)

project (kcov-ut)
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}
                        ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
find_package (LibCRPCUT REQUIRED)
find_package (LibElf REQUIRED)
find_package (LibDwarf REQUIRED)
find_package (GTest REQUIRED)

# ====================================
# project name and version
# ====================================
project (unit-test)
set (TGT ut)

set (${TGT}_SRCS
    ../../src/cobertura-writer.cc
    ../../src/collector.cc
    ../../src/elf.cc
    ../../src/engine-factory.cc
    ../../src/html-writer.cc
    ../../src/output-handler.cc
    ../../src/parser-manager.cc
    ../../src/reporter.cc
    ../../src/utils.cc
    ../../src/writer-base.cc
    main.cc
    tests-collector.cc
    tests-configuration.cc
    tests-elf.cc
    tests-filter.cc
    tests-merge-parser.cc
    tests-reporter.cc
    tests-utils.cc
    tests-writer.cc
    )
set (CMAKE_BUILD_TYPE debug)

set (CMAKE_CXX_FLAGS "-std=c++0x -Wall -D_GLIBCXX_USE_NANOSLEEP")


include_directories(
	../../src/include/
	${GMOCK_INCLUDE_DIRS}
	${GTEST_INCLUDE_DIRS}
	${LIBCRPCUT_INCLUDE_DIRS}
	${LIBELF_INCLUDE_DIRS}
	${LIBDWARF_INCLUDE_DIRS}
	)

link_directories (/home/ska/local/lib)

add_executable (${TGT} ${${TGT}_SRCS})

target_link_libraries(${TGT}
	${LIBCRPCUT_LIBRARIES}
	${GTEST_BOTH_LIBRARIES}
	gmock
	${LIBDWARF_LIBRARIES}
	${LIBELF_LIBRARIES}
	dl
	z
	pthread)

add_executable (same-name-test
	../main.cc
	../subdir/file.c
	../subdir2/file.c
)
  
add_executable (test-binary test-source.c second-source.c)
SET_TARGET_PROPERTIES(test-binary PROPERTIES COMPILE_FLAGS "-nostdinc")
SET_TARGET_PROPERTIES(test-binary PROPERTIES LINK_FLAGS "-nostdlib")
