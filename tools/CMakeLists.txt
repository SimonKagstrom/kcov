cmake_minimum_required (VERSION 2.6)

project (kcov-tools)
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}
                        ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
find_package (LibElf REQUIRED)
find_package (Elfutils REQUIRED)
find_package (PkgConfig REQUIRED)
pkg_check_modules(LIBZ REQUIRED zlib)

# ====================================
# project name and version
# ====================================
set (LINE2ADDR line2addr)

set (${LINE2ADDR}_SRCS
    ../src/capabilities.cc
	../src/configuration.cc
	../src/filter.cc
	../src/gcov.cc
	../src/parsers/elf-parser.cc
	../src/parsers/dummy-address-verifier.cc
	../src/parser-manager.cc
	../src/utils.cc
	line2addr.cc
	)


set (CMAKE_CXX_FLAGS "-std=c++0x -g -Wall -D_GLIBCXX_USE_NANOSLEEP -DKCOV_LIBRARY_PREFIX=${KCOV_LIBRARY_PREFIX}")

include_directories(
	../src/include/
	${LIBELF_INCLUDE_DIRS}
	${LIBDW_INCLUDE_DIRS}
	${LIBZ_INCLUDE_DIRS}
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

add_executable (${LINE2ADDR} ${${LINE2ADDR}_SRCS})

target_link_libraries(${LINE2ADDR}
	${LIBDW_LIBRARIES}
	${LIBELF_LIBRARIES}
	stdc++
	dl
	pthread
	m
	${LIBZ_LIBRARIES})

file ( GLOB kcov-merge kcov-merge )

install (PROGRAMS ${kcov-merge} DESTINATION bin )
