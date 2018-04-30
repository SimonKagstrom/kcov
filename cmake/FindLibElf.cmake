# - Try to find libelf
# Once done this will define
#
#  LIBELF_FOUND - system has libelf
#  LIBELF_INCLUDE_DIRS - the libelf include directory
#  LIBELF_LIBRARIES - Link these to use libelf
#  LIBELF_DEFINITIONS - Compiler switches required for using libelf
#
#  Copyright (c) 2008 Bernhard Walle <bernhard.walle@gmx.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if (LIBELF_LIBRARIES AND LIBELF_INCLUDE_DIRS)
  set (LibElf_FIND_QUIETLY TRUE)
endif (LIBELF_LIBRARIES AND LIBELF_INCLUDE_DIRS)

find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
  set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
  pkg_check_modules(PC_LIBELF QUIET libelf)
endif()

find_path (LIBELF_INCLUDE_DIR
    NAMES
      libelf.h
    HINTS
      ${PC_LIBELF_INCLUDE_DIRS}
    PATHS
      /usr/include
      /usr/include/libelf
      /usr/local/include
      /usr/local/include/libelf
      /opt/local/include
      /opt/local/include/libelf
      /sw/include
      /sw/include/libelf
      ENV CPATH)

find_library (LIBELF_LIBRARY
    NAMES
      elf
    HINTS
      ${PC_LIBELF_LIBRARY_DIRS}
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
      ENV LIBRARY_PATH
      ENV LD_LIBRARY_PATH)

include (FindPackageHandleStandardArgs)


# handle the QUIETLY and REQUIRED arguments and set LIBELF_FOUND to TRUE if all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibElf DEFAULT_MSG
    LIBELF_LIBRARY
    LIBELF_INCLUDE_DIR)


mark_as_advanced(LIBELF_INCLUDE_DIR LIBELF_LIBRARY)

set(LIBELF_LIBRARIES ${LIBELF_LIBRARY} )
set(LIBELF_INCLUDE_DIRS ${LIBELF_INCLUDE_DIR} )
