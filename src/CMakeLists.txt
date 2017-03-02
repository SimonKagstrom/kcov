cmake_minimum_required (VERSION 2.6)

project (kcov)
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}
                        ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
find_package (Threads)
find_package (Bfd)
find_package (Dyninst)

include(TargetArch)
target_architecture(CMAKE_TARGET_ARCHITECTURES)
message(STATUS "Target architectures: ${CMAKE_TARGET_ARCHITECTURES}")

pkg_check_modules(LIBZ REQUIRED zlib)
pkg_check_modules(LIBCURL REQUIRED libcurl)

# ====================================
# project name and version
# ====================================
project (kcov)
set (KCOV kcov)


# See http://eatmyrandom.blogspot.se/2010/06/automate-version-numbering-using-git.html
if( EXISTS "${CMAKE_SOURCE_DIR}/.git" )
       execute_process(COMMAND git --git-dir=${CMAKE_CURRENT_SOURCE_DIR}/../.git describe --abbrev=4 HEAD
               OUTPUT_VARIABLE POD_VERSION
               OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
       execute_process(COMMAND head -n 1 ${CMAKE_CURRENT_SOURCE_DIR}/../ChangeLog
               COMMAND cut -d \( -f 2
               COMMAND cut -d \) -f 1
               OUTPUT_VARIABLE POD_VERSION
               OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

add_custom_command(
   OUTPUT version.c
   COMMAND ${CMAKE_COMMAND} -E echo "const char *kcov_version = \\\"${POD_VERSION}\\\"\;"
  > version.c
)


set (SOLIB kcov_sowrapper)

set (${SOLIB}_SRCS
	solib-parser/phdr_data.c
	solib-parser/lib.c
	)

set (DISASSEMBLER_SRCS
	parsers/dummy-disassembler.cc
)

set (DYNINST_SRCS
)

set (HAS_LIBBFD "0")

if("${CMAKE_TARGET_ARCHITECTURES}" STREQUAL "i386" OR "${CMAKE_TARGET_ARCHITECTURES}" STREQUAL "x86_64")
	if(LIBBFD_FOUND)
		set (HAS_LIBBFD "1")
		set (DISASSEMBLER_SRCS
			parsers/bfd-disassembler.cc
		)
		set (DISASSEMBLER_LIBRARIES
			${LIBBFD_OPCODES_LIBRARY}
			${LIBBFD_BFD_LIBRARY}
			${LIBBFD_IBERTY_LIBRARY}
		)
	endif()
endif()


if(DYNINST_FOUND)
	set (DYNINST_SRCS
		engines/dyninst-engine.cc
	)
endif()

set (coveralls_SRCS writers/coveralls-writer.cc)

if ("${KCOV_STATIC_BUILD}" STREQUAL "1")
	message(STATUS "Building a static binary (no coveralls support)")

	set (LIBCURL_LIBRARIES "")
	# Coveralls doesn't work in a static setting
	set (coveralls_SRCS writers/dummy-coveralls-writer.cc)
	set (CMAKE_EXE_LINKER_FLAGS "-static")
endif()

set (ELF_SRCS
	dummy-solib-handler.cc
)
set (MACHO_SRCS
)
set (SOLIB_generated )
set (DYNINST_SOLIB_generated )

# Linux-specific sources
if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
	find_package (LibElf REQUIRED)
	find_package (Elfutils REQUIRED)
	set (ELF_SRCS
		engines/clang-coverage-engine.cc
		engines/ptrace.cc
		engines/kernel-engine.cc
		parsers/elf.cc
		parsers/elf-parser.cc
		parsers/dwarf.cc
		solib-handler.cc
		solib-parser/phdr_data.c
	)
	set (SOLIB_generated library.cc)
	add_library (${SOLIB} SHARED ${${SOLIB}_SRCS})
	set_target_properties(${SOLIB} PROPERTIES SUFFIX ".so")
	target_link_libraries(${SOLIB} dl)
else()
	find_library(LLDB_LIBRARY
      NAMES
	    LLDB
	  PATHS
        "/usr/lib"
        "/usr/lib64"
        "/usr/local/lib"
        "/usr/local/lib64"
        "/opt/local/lib"
        "/opt/usr/lib64"
		"/Applications/Xcode.app/Contents/SharedFrameworks"
		ENV LIBRARY_PATH
		ENV LD_LIBRARY_PATH
	)
	mark_as_advanced(LLDB_LIBRARY)

	set (MACHO_SRCS
		engines/lldb-engine.cc
	)
endif ()

set (${KCOV}_SRCS
    capabilities.cc
    collector.cc
    configuration.cc
    engine-factory.cc
    engines/bash-engine.cc
    engines/dyninst-file-format.cc
    engines/gcov-engine.cc
    engines/python-engine.cc
    filter.cc
    gcov.cc
    main.cc
    merge-file-parser.cc
    output-handler.cc
    ${DISASSEMBLER_SRCS}
    parser-manager.cc
    reporter.cc
    source-file-cache.cc
    utils.cc
    writers/cobertura-writer.cc
    writers/json-writer.cc
    ${coveralls_SRCS}
    writers/html-writer.cc
    writers/sonarqube-xml-writer.cc
    writers/writer-base.cc
    ${ELF_SRCS}
    ${MACHO_SRCS}
    include/capabilities.hh
    include/gcov.hh
    include/reporter.hh
    include/collector.hh
    include/generated-data-base.hh
    include/solib-handler.hh
    include/configuration.hh
    include/lineid.hh
    include/swap-endian.hh
    include/engine.hh
    include/manager.hh
    include/utils.hh
    include/file-parser.hh
    include/output-handler.hh
    include/writer.hh
    include/filter.hh
    include/phdr_data.h
    )

# Should be some better way of doing this...
set (KCOV_DYNINST_SRCS
    capabilities.cc
    collector.cc
    configuration.cc
    dummy-solib-handler.cc
    engine-factory.cc
    engines/bash-engine.cc
    engines/dyninst-engine.cc
    engines/dyninst-file-format.cc
    engines/gcov-engine.cc
    engines/python-engine.cc
    filter.cc
    gcov.cc
    main.cc
    merge-file-parser.cc
    output-handler.cc
    parser-manager.cc
    reporter.cc
    source-file-cache.cc
    utils.cc
    writers/cobertura-writer.cc
    writers/json-writer.cc
    ${coveralls_SRCS}
    writers/html-writer.cc
    writers/sonarqube-xml-writer.cc
    writers/writer-base.cc
    include/capabilities.hh
    include/gcov.hh
    include/reporter.hh
    include/collector.hh
    include/generated-data-base.hh
    include/solib-handler.hh
    include/configuration.hh
    include/lineid.hh
    include/swap-endian.hh
    include/engine.hh
    include/manager.hh
    include/utils.hh
    include/file-parser.hh
    include/output-handler.hh
    include/writer.hh
    include/filter.hh
    include/phdr_data.h
)

set (KCOV_LIBRARY_PREFIX "/tmp")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g -Wall -D_GLIBCXX_USE_NANOSLEEP -DKCOV_LIBRARY_PREFIX=${KCOV_LIBRARY_PREFIX} -DKCOV_HAS_LIBBFD=${HAS_LIBBFD}")

include_directories(
	include/
	../external/lldb/include/
	${LIBELF_INCLUDE_DIRS}
	${LIBDW_INCLUDE_DIRS}
	${LIBZ_INCLUDE_DIRS}
	${LIBCURL_INCLUDE_DIRS}
	${DYNINST_INCLUDE_DIRS}
	)

link_directories (/home/ska/local/lib)

add_library (bash_execve_redirector SHARED engines/bash-execve-redirector.c)
set_target_properties(bash_execve_redirector PROPERTIES SUFFIX ".so")

target_link_libraries(bash_execve_redirector dl)

add_custom_command(
   OUTPUT library.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py lib${SOLIB}.so __library > library.cc
   DEPENDS ${SOLIB} ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

add_custom_command(
   OUTPUT dyninst-binary-library.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py libkcov-binary-dyninst.so __dyninst_binary_library > dyninst-binary-library.cc
   DEPENDS kcov-binary-dyninst ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

add_custom_command(
   OUTPUT bash-redirector-library.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py $<TARGET_FILE:bash_execve_redirector> bash_redirector_library > bash-redirector-library.cc
   DEPENDS bash_execve_redirector ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

add_custom_command(
   OUTPUT python-helper.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py ${CMAKE_CURRENT_SOURCE_DIR}/engines/python-helper.py  python_helper > python-helper.cc
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engines/python-helper.py ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

add_custom_command(
   OUTPUT bash-helper.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
     ${CMAKE_CURRENT_SOURCE_DIR}/engines/bash-helper.sh bash_helper
     ${CMAKE_CURRENT_SOURCE_DIR}/engines/bash-helper-debug-trap.sh bash_helper_debug_trap
   > bash-helper.cc
   DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/engines/bash-helper.sh
     ${CMAKE_CURRENT_SOURCE_DIR}/engines/bash-helper-debug-trap.sh
     ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
   )

add_custom_command(
   OUTPUT html-data-files.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin-to-c-source.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/bcov.css css_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/amber.png icon_amber
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/glass.png icon_glass
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/source-file.html source_file_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/index.html index_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/handlebars.js handlebars_text
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/js/handlebars.js
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

add_executable (${KCOV} ${${KCOV}_SRCS} ${SOLIB_generated} bash-redirector-library.cc python-helper.cc bash-helper.cc html-data-files.cc version.c)

target_link_libraries(${KCOV}
	${LIBDW_LIBRARIES}
	${LIBELF_LIBRARIES}
	stdc++
	${LIBCURL_LIBRARIES}
	m
	${DISASSEMBLER_LIBRARIES}
	${LIBZ_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	dl
	${LLDB_LIBRARY}
)

if(DYNINST_FOUND)
	add_library (kcov-binary-dyninst SHARED engines/dyninst-binary-lib.cc engines/dyninst-file-format.cc utils.cc)
	set_target_properties(kcov-binary-dyninst PROPERTIES SUFFIX ".so")

    add_executable (kcov-system ${KCOV_DYNINST_SRCS} ${SOLIB_generated} bash-redirector-library.cc dyninst-binary-library.cc python-helper.cc bash-helper.cc html-data-files.cc version.c)

    target_link_libraries(kcov-system
        ${DYNINST_LIBRARIES}
        ${LIBELF_LIBRARIES}
        stdc++
        ${LIBCURL_LIBRARIES}
        m
        ${DISASSEMBLER_LIBRARIES}
        ${LIBZ_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        dl
        ${LLDB_LIBRARY}
)
endif()


install (TARGETS ${PROJECT_NAME} ${INSTALL_TARGETS_PATH})
