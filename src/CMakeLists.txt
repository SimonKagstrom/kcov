cmake_minimum_required (VERSION 2.6)

project (kcov)
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}
                        ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
find_package (LibElf REQUIRED)
find_package (LibDwarf REQUIRED)

# ====================================
# project name and version
# ====================================
project (unit-test)
set (TGT kcov)

set (SOLIB kcov_sowrapper)

set (${SOLIB}_SRCS
	solib-parser/phdr_data.c
	solib-parser/lib.c
	)

set (${TGT}_SRCS
    cobertura-writer.cc
    collector.cc
    configuration.cc
    elf.cc
    filter.cc
    html-writer.cc
    main.cc
    output-handler.cc
    ptrace.cc
    reporter.cc
    solib-parser/phdr_data.c
    utils.cc
    writer-base.cc
    )
set (CMAKE_BUILD_TYPE debug)

set (CMAKE_CXX_FLAGS "-std=c++0x -Wall -D_GLIBCXX_USE_NANOSLEEP")


include_directories(
	include/
	${LIBELF_INCLUDE_DIRS}
	${LIBDWARF_INCLUDE_DIRS}
	)

link_directories (/home/ska/local/lib)

add_library (${SOLIB} SHARED ${${SOLIB}_SRCS})
add_executable (${TGT} ${${TGT}_SRCS})

target_link_libraries(${TGT}
	${LIBDWARF_LIBRARIES}
	${LIBELF_LIBRARIES}
	dl
	pthread)
