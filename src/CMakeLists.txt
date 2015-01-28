cmake_minimum_required (VERSION 2.6)

project (kcov)
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}
                        ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
find_package (LibElf REQUIRED)
find_package (Elfutils REQUIRED)
pkg_check_modules(LIBZ REQUIRED zlib)
pkg_check_modules(LIBCURL REQUIRED libcurl)

# ====================================
# project name and version
# ====================================
project (kcov)
set (KCOV kcov)
set (LINE2ADDR line2addr)
#set (CMAKE_EXE_LINKER_FLAGS "-static")

set (SOLIB kcov_sowrapper)

set (${SOLIB}_SRCS
	solib-parser/phdr_data.c
	solib-parser/lib.c
	)

set (${KCOV}_SRCS
    capabilities.cc
    collector.cc
    configuration.cc
    engine-factory.cc
    engines/bash-engine.cc
    engines/gcov-engine.cc
    engines/kernel-engine.cc
    engines/ptrace.cc
    engines/python-engine.cc
    filter.cc
    gcov.cc
    main.cc
    merge-file-parser.cc
    output-handler.cc
    parsers/elf-parser.cc
    parser-manager.cc
    reporter.cc
    solib-handler.cc
    solib-parser/phdr_data.c
    utils.cc
    writers/cobertura-writer.cc
    writers/coveralls-writer.cc
    writers/html-writer.cc
    writers/writer-base.cc
    )

set (${LINE2ADDR}_SRCS
    capabilities.cc
	configuration.cc
	filter.cc
	gcov.cc
	line2addr.cc
	parsers/elf-parser.cc
	parser-manager.cc
	utils.cc
	)


set (KCOV_LIBRARY_PREFIX "/tmp")

set (CMAKE_CXX_FLAGS "-std=c++0x -fsanitize=address,undefined -g -Wall -D_GLIBCXX_USE_NANOSLEEP -DKCOV_LIBRARY_PREFIX=${KCOV_LIBRARY_PREFIX}")

include_directories(
	include/
	${LIBELF_INCLUDE_DIRS}
	${LIBDW_INCLUDE_DIRS}
	${LIBZ_INCLUDE_DIRS}
	${LIBCURL_INCLUDE_DIRS}
	)

link_directories (/home/ska/local/lib)

add_library (${SOLIB} SHARED ${${SOLIB}_SRCS})

target_link_libraries(${SOLIB} dl)

add_custom_command(
   OUTPUT library.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py lib${SOLIB}.so __library > library.cc
   DEPENDS ${SOLIB} ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

add_custom_command(
   OUTPUT python-helper.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py ${CMAKE_CURRENT_SOURCE_DIR}/engines/python-helper.py  python_helper > python-helper.cc
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engines/python-helper.py ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

add_custom_command(
   OUTPUT bash-helper.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py ${CMAKE_CURRENT_SOURCE_DIR}/engines/bash-helper.sh bash_helper > bash-helper.cc
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engines/bash-helper.sh ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

add_custom_command(
   OUTPUT html-data-files.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/bcov.css css_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/amber.png icon_amber
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/glass.png icon_glass
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/source-file.html source_file_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/index.html index_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/tempo.min.js tempo_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/kcov.js kcov_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/jquery.min.js jquery_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/jquery.tablesorter.min.js tablesorter_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/jquery.tablesorter.widgets.min.js tablesorter_widgets_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/tablesorter-theme.css tablesorter_theme_text
   > html-data-files.cc
   DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/bcov.css
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/amber.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/glass.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/source-file.html
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/tempo.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/kcov.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/jquery.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/jquery.tablesorter.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/jquery.tablesorter.widgets.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/tablesorter-theme.css
    ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

# Reference: http://www.cmake.org/Wiki/CMake_RPATH_handling
if(SPECIFY_RPATH)
	set (CMAKE_SKIP_BUILD_RPATH  FALSE)
	set (CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
	set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	# the RPATH to be used when installing, but only if it's not a system directory
	LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
	IF("${isSystemDir}" STREQUAL "-1")
	  SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
	ENDIF("${isSystemDir}" STREQUAL "-1")
endif(SPECIFY_RPATH)

add_executable (${KCOV} ${${KCOV}_SRCS} library.cc python-helper.cc bash-helper.cc html-data-files.cc)
add_executable (${LINE2ADDR} ${${LINE2ADDR}_SRCS})

target_link_libraries(${KCOV}
	${LIBDW_LIBRARIES}
	${LIBELF_LIBRARIES}
	stdc++
	dl
	pthread
	${LIBCURL_LIBRARIES}
	m
	${LIBZ_LIBRARIES})


target_link_libraries(${LINE2ADDR}
	${LIBDW_LIBRARIES}
	${LIBELF_LIBRARIES}
	stdc++
	dl
	pthread
	m
	${LIBZ_LIBRARIES})

install (TARGETS ${PROJECT_NAME} ${INSTALL_TARGETS_PATH})

file ( GLOB kcov-merge kcov-merge )

install (PROGRAMS ${kcov-merge} DESTINATION bin )
