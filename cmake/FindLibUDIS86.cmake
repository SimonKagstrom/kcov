# - Try to find libudis
# Once done this will define
#
#  LIBUDIS86_FOUND - system has libudis86
#  LIBUDIS86_INCLUDE_DIRS - the libudis86 include directory
#  LIBUDIS86_LIBRARIES - Link these to use libudis86
#  LIBUDIS86_DEFINITIONS - Compiler switches required for using libudis86
#
# Based on:
#
#  Copyright (c) 2008 Bernhard Walle <bernhard.walle@gmx.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (LIBUDIS86_LIBRARIES AND LIBUDIS86_INCLUDE_DIRS)
  set (LibUDIS86_FIND_QUIETLY TRUE)
endif (LIBUDIS86_LIBRARIES AND LIBUDIS86_INCLUDE_DIRS)

find_path (LIBUDIS86_INCLUDE_DIRS
    NAMES
      udis86.h
    PATHS
      /usr/include
      /usr/include/udis86
      /usr/local/include
      /usr/local/include/udis86
      /opt/local/include
      /opt/local/include/udis86
      /home/ska/local/include
      ENV CPATH)

find_library (LIBUDIS86_LIBRARIES
    NAMES
      udis86
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /home/ska/local/lib
      ENV LIBRARY_PATH
      ENV LD_LIBRARY_PATH)

include (FindPackageHandleStandardArgs)


# handle the QUIETLY and REQUIRED arguments and set LIBUDIS86_FOUND to TRUE if all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibUDIS86 DEFAULT_MSG
    LIBUDIS86_LIBRARIES
    LIBUDIS86_INCLUDE_DIRS)


mark_as_advanced(LIBUDIS86_INCLUDE_DIRS LIBUDIS86_LIBRARIES)
