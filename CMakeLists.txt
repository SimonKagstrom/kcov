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
find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB2 REQUIRED glib-2.0)
include_directories (${GLIB2_INCLUDE_DIRS})
pkg_check_modules (ELF libelf)
if (ELF_FOUND)
    include_directories (${ELF_INCLUDE_DIRS})
else (ELF_FOUND)
    try_compile (TEST_ELF
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/libelf/
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libelf/
        test_elf)
    if (NOT TEST_ELF)
        message (FATAL_ERROR "*** Failed to find working libelf!")
    endif (NOT TEST_ELF)
endif (ELF_FOUND)


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
