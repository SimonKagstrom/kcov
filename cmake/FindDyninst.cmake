# - Try to find dyninst
# Once done this will define
#
#  DYNINST_FOUND - system has dyninst
#  DYNINST_INCLUDE_DIRS - the dyninst include directory
#  DYNINST_LIBRARIES - Link these to use dyninst
#  DYNINST_DEFINITIONS - Compiler switches required for using dyninst
#

if (DYNINST_LIBRARIES AND DYNINST_INCLUDE_DIRS)
  set (Dyninst_FIND_QUIETLY TRUE)
endif (DYNINST_LIBRARIES AND DYNINST_INCLUDE_DIRS)

find_path (DYNINST_INCLUDE_DIR
    NAMES
      BPatch.h
    PATHS
      /usr/include/dyninst
      /usr/local/include/dyninst
      /opt/local/include/dyninst
      /sw/include/dyninst
      /sw/include/dyninst
      /usr/local/include
      ENV CPATH) # PATH and INCLUDE will also work
if (DYNINST_INCLUDE_DIR)
    set (DYNINST_INCLUDE_DIRS  ${DYNINST_INCLUDE_DIR})
endif (DYNINST_INCLUDE_DIR)

find_library (DYNINST_LIBRARIES
    NAMES
      dyninstAPI
    PATHS
      /usr/lib
      /usr/lib64
      /usr/local/lib
      /opt/local/lib
      /usr/local/lib64
      /opt/local/lib64
      /sw/lib
      /usr/lib/dyninst
      /usr/lib64/dyninst
      /usr/local/lib/dyninst
      /opt/local/lib/dyninst
      /usr/local/lib64/dyninst
      /opt/local/lib64/dyninst
      ENV LIBRARY_PATH   # PATH and LIB will also work
      ENV LD_LIBRARY_PATH)

include (FindPackageHandleStandardArgs)


# handle the QUIETLY and REQUIRED arguments and set DYNINST_FOUND to TRUE
# if all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DYNINST DEFAULT_MSG
    DYNINST_LIBRARIES
    DYNINST_INCLUDE_DIR)

mark_as_advanced(DYNINST_INCLUDE_DIR DYNINST_LIBRARIES)
