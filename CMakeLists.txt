cmake_minimum_required (VERSION 2.6)


# ====================================
# project name and version
# ====================================
project (kcov)
set (PROJECT_VERSION_MAJOR  4)
set (PROJECT_VERSION_MINOR  0)
set (PROJECT_VERSION_PATCH  0)
set (PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")


if(NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE  Debug)
endif(NOT CMAKE_BUILD_TYPE)

# ====================================
# configuring
# ====================================
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}
                        ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB2 REQUIRED glib-2.0)
find_package (LibElf REQUIRED)
find_package (LibDwarf REQUIRED)


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
