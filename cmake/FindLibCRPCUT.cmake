# - Try to find libcrpcut
# Once done this will define
#
#  LIBCRPCUT_FOUND - system has libcrpcut
#  LIBCRPCUT_INCLUDE_DIRS - the libcrpcut include directory
#  LIBCRPCUT_LIBRARIES - Link these to use libcrpcut
#  LIBCRPCUT_DEFINITIONS - Compiler switches required for using libcrpcut
#
# Based on:
#
#  Copyright (c) 2008 Bernhard Walle <bernhard.walle@gmx.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (LIBCRPCUT_LIBRARIES AND LIBCRPCUT_INCLUDE_DIRS)
  set (LibCRPCUT_FIND_QUIETLY TRUE)
endif (LIBCRPCUT_LIBRARIES AND LIBCRPCUT_INCLUDE_DIRS)

find_path (LIBCRPCUT_INCLUDE_DIRS
    NAMES
      crpcut.hpp
    PATHS
      /usr/include
      /usr/include/crpcut
      /usr/local/include
      /usr/local/include/crpcut
      /opt/local/include
      /opt/local/include/crpcut
      /home/ska/local/include
      ENV CPATH)

find_library (LIBCRPCUT_LIBRARIES
    NAMES
      crpcut
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /home/ska/local/lib
      ENV LIBRARY_PATH
      ENV LD_LIBRARY_PATH)

include (FindPackageHandleStandardArgs)


# handle the QUIETLY and REQUIRED arguments and set LIBCRPCUT_FOUND to TRUE if all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibCRPCUT DEFAULT_MSG
    LIBCRPCUT_LIBRARIES
    LIBCRPCUT_INCLUDE_DIRS)


mark_as_advanced(LIBCRPCUT_INCLUDE_DIRS LIBCRPCUT_LIBRARIES)
