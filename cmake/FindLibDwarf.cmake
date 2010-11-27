# - Try to find libdwarf
# Once done this will define
#
#  LIBDWARF_FOUND - system has libdwarf
#  LIBDWARF_INCLUDE_DIRS - the libdwarf include directory
#  LIBDWARF_LIBRARIES - Link these to use libdwarf
#  LIBDWARF_DEFINITIONS - Compiler switches required for using libdwarf
#

# Locate libelf library at first
if (NOT LIBELF_FOUND)
   find_package (LibElf REQUIRED)
endif (NOT LIBELF_FOUND)

if (LIBDWARF_LIBRARIES AND LIBDWARF_INCLUDE_DIRS)
  set (LibDwarf_FIND_QUIETLY TRUE)
endif (LIBDWARF_LIBRARIES AND LIBDWARF_INCLUDE_DIRS)

find_path (LIBDWARF_INCLUDE_DIRS
    NAMES
      libdwarf.h
    PATHS
      /usr/include
      /usr/include/libdwarf
      /usr/local/include
      /usr/local/include/libdwarf
      /opt/local/include
      /opt/local/include/libdwarf
      /sw/include
      /sw/include/libdwarf
      ENV CPATH)

find_library (LIBDWARF_LIBRARIES
    NAMES
      dwarf
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
      ENV LIBRARY_PATH
      ENV LD_LIBRARY_PATH)

include (FindPackageHandleStandardArgs)


# handle the QUIETLY and REQUIRED arguments and set LIBDWARF_FOUND to TRUE
# if all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibDwarf DEFAULT_MSG
    LIBDWARF_LIBRARIES
    LIBDWARF_INCLUDE_DIRS)


mark_as_advanced(LIBDWARF_INCLUDE_DIRS LIBDWARF_LIBRARIES)
