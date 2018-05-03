cmake_minimum_required (VERSION 2.8.4)
project (kcov)

set (PROJECT_VERSION_MAJOR 35)
set (PROJECT_VERSION_MINOR 0)
set (PROJECT_VERSION_PATCH 0)
set (PROJECT_VERSION
     "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

if (POLICY CMP0042)
  # MACOSX_RPATH is enabled by default.
  cmake_policy (SET CMP0042 NEW)
endif (POLICY CMP0042)

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

option (SPECIFY_RPATH  "Specify RPATH for installed executables" OFF)
mark_as_advanced (SPECIFY_RPATH)

# ====================================
# default install paths for targets
# ====================================
include (CPack.local.cmake OPTIONAL)

set (KCOV_INSTALL_BINDIR "bin" CACHE STRING "Install destination for user executables")
set (KCOV_INSTALL_DOCDIR "share/doc/kcov" CACHE STRING "Install destination for documentation")
set (KCOV_INSTALL_MANDIR "share/man" CACHE STRING "Install destination for man documentation")
mark_as_advanced (KCOV_INSTALL_BINDIR KCOV_INSTALL_DOCDIR KCOV_INSTALL_MANDIR)

install (FILES ChangeLog COPYING COPYING.externals
         DESTINATION "${KCOV_INSTALL_DOCDIR}")

# ====================================
# build all
# ====================================
add_subdirectory (src)
add_subdirectory (doc)
