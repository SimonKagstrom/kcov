cmake_minimum_required (VERSION 2.6)


# ====================================
# project name and version
# ====================================
project (kcov)

if(NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE  Release)
endif(NOT CMAKE_BUILD_TYPE)

add_definitions(-DPACAKGE)
add_definitions(-DPACKAGE_VERSION)
# ====================================
# configuring
# ====================================
set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}
                        ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package (PkgConfig REQUIRED)

set (SPECIFY_RPATH OFF CACHE BOOL "Specify RPATH for installed executables")

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
