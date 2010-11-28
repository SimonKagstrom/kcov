cmake_minimum_required (VERSION 2.8)


# ====================================
# project name and version
# ====================================
project (kcov)
set (PACKAGE_VERSION_MAJOR  4)
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
pkg_check_modules (LIBDWARF libdwarf)
if (NOT LIBDWARF_FOUND)
    find_package (LibDwarf REQUIRED)
endif (NOT LIBDWARF_FOUND)


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
