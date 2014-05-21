cmake_minimum_required (VERSION 2.6)

project (kcov)
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}
                        ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
find_package (LibElf REQUIRED)
find_package (LibDwarf REQUIRED)
pkg_check_modules(LIBZ REQUIRED zlib)

# ====================================
# project name and version
# ====================================
project (kcov)
set (KCOV kcov)
set (KCOV_PROFILER kcov-profiler)
set (LINE2ADDR line2addr)
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
    engines/kernel-engine.cc
    engines/ptrace.cc
    engines/python-engine.cc
    filter.cc
    html-writer.cc
    main.cc
    merge-file-parser.cc
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

set (${LINE2ADDR}_SRCS
	configuration.cc
	elf.cc
	line2addr.cc
	parser-manager.cc
	utils.cc
	)


set (KCOV_LIBRARY_PREFIX "/tmp")

set (CMAKE_CXX_FLAGS "-std=c++0x -Wall -D_GLIBCXX_USE_NANOSLEEP -DKCOV_LIBRARY_PREFIX=${KCOV_LIBRARY_PREFIX}")

include_directories(
	include/
	${LIBELF_INCLUDE_DIRS}
	${LIBDWARF_INCLUDE_DIRS}
	${LIBZ_INCLUDE_DIRS}
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
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engines/python-helper.py ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

add_custom_command(
   OUTPUT html-data-files.c
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/bcov.css css_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/amber.png icon_amber
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/glass.png icon_glass
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/emerald.png icon_emerald
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/ruby.png icon_ruby
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/snow.png icon_snow
   > html-data-files.c
   DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/bcov.css
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/amber.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/glass.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/emerald.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/ruby.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/snow.png
    ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )


add_executable (${KCOV} ${${KCOV}_SRCS} library.c python-helper.c html-data-files.c)
add_executable (${LINE2ADDR} ${${LINE2ADDR}_SRCS})
#add_executable (${KCOV_PROFILER} ${${KCOV_PROFILER}_SRCS})

target_link_libraries(${KCOV}
	${LIBDWARF_LIBRARIES}
	${LIBELF_LIBRARIES}
	stdc++
	dl
	pthread
	m
	${LIBZ_LIBRARIES})


target_link_libraries(${LINE2ADDR}
	${LIBDWARF_LIBRARIES}
	${LIBELF_LIBRARIES}
	stdc++
	dl
	pthread
	m
	${LIBZ_LIBRARIES})

install (TARGETS ${PROJECT_NAME} ${INSTALL_TARGETS_PATH})
