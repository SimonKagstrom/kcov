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
  # in cache already
  set(LIBELF_FOUND TRUE)
else (LIBELF_LIBRARIES AND LIBELF_INCLUDE_DIRS)
  find_path(LIBELF_INCLUDE_DIR
    NAMES
      gelf.h
    PATHS
      /usr/include
      /usr/include/libelf
      /usr/local/include
      /usr/local/include/libelf
      /opt/local/include
      /opt/local/include/libelf
      /sw/include
      /sw/include/libelf
  )

  find_library(LIBELF_LIBRARY
    NAMES
      elf
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (LIBELF_LIBRARY)
    set(LIBELF_FOUND TRUE)
  endif (LIBELF_LIBRARY)

  set(LIBELF_INCLUDE_DIRS
    ${LIBELF_INCLUDE_DIR}
  )

  if (LIBELF_FOUND)
    set(LIBELF_LIBRARIES
      ${LIBELF_LIBRARIES}
      ${LIBELF_LIBRARY}
    )
  endif (LIBELF_FOUND)

  if (LIBELF_INCLUDE_DIRS AND LIBELF_LIBRARIES)
     set(LIBELF_FOUND TRUE)
  endif (LIBELF_INCLUDE_DIRS AND LIBELF_LIBRARIES)

  if (LIBELF_FOUND)
    if (NOT libelf_FIND_QUIETLY)
      message(STATUS "Found libelf: ${LIBELF_LIBRARIES}")
    endif (NOT libelf_FIND_QUIETLY)
  else (LIBELF_FOUND)
    if (libelf_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libelf")
    endif (libelf_FIND_REQUIRED)
  endif (LIBELF_FOUND)

  # show the LIBELF_INCLUDE_DIRS and LIBELF_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBELF_INCLUDE_DIRS LIBELF_LIBRARIES)
  mark_as_advanced(LIBELF_INCLUDE_DIR LIBELF_LIBRARY)

endif (LIBELF_LIBRARIES AND LIBELF_INCLUDE_DIRS)

