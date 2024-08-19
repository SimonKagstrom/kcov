cmake_minimum_required (VERSION 3.12)
project (kcov)


# See https://eatmyrandom.blogspot.com/2010/06/automate-version-numbering-using-git.html
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
    find_program (GIT_EXECUTABLE NAMES git)
    if (GIT_EXECUTABLE)
        message (STATUS "Found Git: ${GIT_EXECUTABLE}")
    else (GIT_EXECUTABLE)
        message (FATAL_ERROR "Could NOT find Git")
    endif (GIT_EXECUTABLE)

    execute_process (COMMAND "${GIT_EXECUTABLE}"
            "--git-dir=${CMAKE_SOURCE_DIR}/.git"
            describe
            --abbrev=4
            --tags
            HEAD
        OUTPUT_VARIABLE POD_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    message (STATUS "kcov version: ${POD_VERSION}")
else (EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process (COMMAND head
            -n 1
            "${CMAKE_SOURCE_DIR}/ChangeLog"
        COMMAND cut -d \( -f 2
        COMMAND cut -d \) -f 1
        OUTPUT_VARIABLE POD_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    message (STATUS "kcov version: ${POD_VERSION} (from changelog)")
endif (EXISTS "${CMAKE_SOURCE_DIR}/.git")

# Example POD_VERSION format: v42-148-g29ae1
string(REGEX REPLACE "v([0-9]+).*" "\\1" PROJECT_VERSION_MAJOR ${POD_VERSION})

# Set the PROJECT_VERSION_MAJOR with the extracted version
set (PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
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

install (FILES ChangeLog COPYING COPYING.spdx
         DESTINATION "${KCOV_INSTALL_DOCDIR}")

# ====================================
# build all
# ====================================
add_subdirectory (src)
add_subdirectory (doc)
