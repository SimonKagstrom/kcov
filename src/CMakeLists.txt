cmake_minimum_required (VERSION 2.6)

project (kcov)
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}
                        ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
find_package (LibElf REQUIRED)
find_package (LibDwarf REQUIRED)

# ====================================
# project name and version
# ====================================
project (kcov)
set (KCOV kcov)
set (KCOV_PROFILER kcov-profiler)
#set (CMAKE_EXE_LINKER_FLAGS "-static")

set (SOLIB kcov_sowrapper)

set (${SOLIB}_SRCS
	solib-parser/phdr_data.c
	solib-parser/lib.c
	)

set (${KCOV}_SRCS
    cobertura-writer.cc
    collector.cc
    configuration.cc
    elf.cc
    engine-factory.cc
    engines/ptrace.cc
    engines/python-engine.cc
    filter.cc
    html-writer.cc
    main.cc
    output-handler.cc
    parser-manager.cc
    reporter.cc
    solib-parser/phdr_data.c
    utils.cc
    writer-base.cc
    )

set (${KCOV_PROFILER}_SRCS
    cobertura-writer.cc
    configuration.cc
    elf.cc
    filter.cc
    html-writer.cc
    output-handler.cc
    profiler-collector.cc
    profiler-main.cc
    reporter.cc
    utils.cc
    writer-base.cc
    )


set (KCOV_LIBRARY_PREFIX "/tmp")

set (CMAKE_CXX_FLAGS "-std=c++0x -Wall -D_GLIBCXX_USE_NANOSLEEP -DKCOV_LIBRARY_PREFIX=${KCOV_LIBRARY_PREFIX}")

include_directories(
	include/
	${LIBELF_INCLUDE_DIRS}
	${LIBDWARF_INCLUDE_DIRS}
	)

link_directories (/home/ska/local/lib)

add_library (${SOLIB} SHARED ${${SOLIB}_SRCS})

add_custom_command(
   OUTPUT library.c
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py lib${SOLIB}.so __library > library.c
   DEPENDS ${SOLIB} ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

add_custom_command(
   OUTPUT python-helper.c
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py ${CMAKE_CURRENT_SOURCE_DIR}/engines/python-helper.py  python_helper > python-helper.c
   DEPENDS ${SOLIB} ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )


add_executable (${KCOV} ${${KCOV}_SRCS} library.c python-helper.c)
#add_executable (${KCOV_PROFILER} ${${KCOV_PROFILER}_SRCS})

target_link_libraries(${KCOV}
	${LIBDWARF_LIBRARIES}
	${LIBELF_LIBRARIES}
	stdc++
	dl
	pthread
	m
	z)

install (TARGETS ${PROJECT_NAME} ${INSTALL_TARGETS_PATH})
