# - Try to find dwarfutils

if (DWARFUTILS_LIBRARIES AND DWARFUTILS_INCLUDE_DIRS)
  set (DWARFUTILS_FIND_QUIETLY ON)
endif (DWARFUTILS_LIBRARIES AND DWARFUTILS_INCLUDE_DIRS)

find_path (DWARFUTILS_INCLUDE_DIR
    NAMES
        libdwarf.h
    PATHS
        /usr/local/opt/dwarfutils/include/libdwarf-2/
        /opt/homebrew/opt/dwarfutils/include/libdwarf-2/
    ENV CPATH)

find_library (DWARFUTILS_LIBRARY
    NAMES
        dwarf
    PATHS
        /usr/local/opt/dwarfutils/lib/
        /opt/homebrew/opt/dwarfutils/lib/
    ENV LIBRARY_PATH
    ENV LD_LIBRARY_PATH)

include (FindPackageHandleStandardArgs)


# handle the QUIETLY and REQUIRED arguments and set DWARFUTILS_FOUND to TRUE if all listed variables are TRUE
find_package_handle_standard_args(dwarfutils DEFAULT_MSG
    DWARFUTILS_LIBRARY
    DWARFUTILS_INCLUDE_DIR)

mark_as_advanced(DWARFUTILS_INCLUDE_DIR DWARFUTILS_LIBRARY)

set(DWARFUTILS_LIBRARIES ${DWARFUTILS_LIBRARY} )
set(DWARFUTILS_INCLUDE_DIRS ${DWARFUTILS_INCLUDE_DIR} )
