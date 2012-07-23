cmake_minimum_required (VERSION 2.6)


# ====================================
# project name and version
# ====================================
project (kcov)
set (PROJECT_VERSION_MAJOR  11)
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


# ====================================
# default install paths for targets
# ====================================
include (CPack.local.cmake OPTIONAL)
set (INSTALL_TARGETS_PATH RUNTIME DESTINATION bin
                          LIBRARY DESTINATION lib${LIB_SUFFIX}
                          ARCHIVE DESTINATION lib${LIB_SUFFIX})
if (NOT MAN_DIR)
  set (MAN_DIR share/man)
endif (NOT MAN_DIR)
set (INSTALL_MAN_PATH  DESTINATION ${MAN_DIR})


# ====================================
# build all
# ====================================
add_subdirectory (src)
add_subdirectory (doc)
