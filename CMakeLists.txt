cmake_minimum_required (VERSION 2.6)


# ====================================
# project name and version
# ====================================
project (kcov)

set (PROJECT_VERSION_MAJOR 35)
set (PROJECT_VERSION_MINOR 0)
set (PROJECT_VERSION_PATCH 0)
set (PROJECT_VERSION
     "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

if(POLICY CMP0042)
  # MACOSX_RPATH is enabled by default.
  cmake_policy (SET CMP0042 NEW)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set (CMAKE_BUILD_TYPE Release CACHE STRING
       "Choose the type of build, options are None Debug Release RelWithDebInfo MinSizeRel"
       FORCE)
  set_property (CACHE CMAKE_BUILD_TYPE PROPERTY
                STRINGS None Debug Release RelWithDebInfo MinSizeRel)
endif (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

add_definitions(-DPACKAGE)
add_definitions(-DPACKAGE_VERSION)
# ====================================
# configuring
# ====================================
list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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
