cmake_minimum_required (VERSION 2.8)


# ====================================
# project name and version
# ====================================
project (kcov)
set (PACKAGE_VERSION_MAJOR  3)
set (PACKAGE_VERSION_MINOR  0)
set (PACKAGE_VERSION_PATCH  0)


# ====================================
# configuring
# ====================================
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}
                        ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB2 REQUIRED glib-2.0)
pkg_check_modules (LIBELF libelf)
if (NOT LIBELF_FOUND)
    find_package (LibElf REQUIRED)
endif (NOT LIBELF_FOUND)
pkg_check_modules (DWARF libdwarf)
if (DWARF_FOUND)
    include_directories (${DWARF_INCLUDE_DIRS})
else (DWARF_FOUND)
    message (STATUS "checking for 'libdwarf'")
    try_compile (TEST_DWARF
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/libdwarf/
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libdwarf/
        test_dwarf
        CMAKE_FLAGS
        -DLIBELF_INCLUDE_DIRS=${LIBELF_INCLUDE_DIRS}
        -DLIBELF_LIBRARIES=${LIBELF_LIBRARIES})
    if (NOT TEST_DWARF)
        message (FATAL_ERROR "*** Failed to find working libdwarf!")
    else (NOT TEST_DWARF)
        message (STATUS "  found libdwarf")
    endif (NOT TEST_DWARF)
endif (DWARF_FOUND)


# ====================================
# default install paths for targets
# ====================================
include (CPack.local.cmake OPTIONAL)
set (INSTALL_TARGETS_DEFAULT_ARGS RUNTIME DESTINATION bin
                                  LIBRARY DESTINATION lib
                                  ARCHIVE DESTINATION lib)


# ====================================
# build all
# ====================================
add_subdirectory (src)
