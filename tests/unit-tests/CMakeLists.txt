cmake_minimum_required (VERSION 2.8.4)
project (kcov-unit-tests)
list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

if (POLICY CMP0042)
    # MACOSX_RPATH is enabled by default.
    cmake_policy (SET CMP0042 NEW)
endif (POLICY CMP0042)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set (CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build, options are None Debug Release RelWithDebInfo MinSizeRel"
        FORCE
    )
    set_property (CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS None Debug Release RelWithDebInfo MinSizeRel
    )
endif (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

find_package (LibCRPCUT REQUIRED)
find_package (LibElf REQUIRED)
find_package (Elfutils REQUIRED)

set (CMAKE_THREAD_PREFER_PTHREAD ON)
find_package (Threads REQUIRED)

find_package (ZLIB REQUIRED)

find_library (DL_LIBRARY NAMES dl)
if (NOT DL_LIBRARY)
    message (FATAL_ERROR "Could NOT find dl library")
endif (NOT DL_LIBRARY)

set (TGT ut)

set (${TGT}_SRCS
    ../../src/capabilities.cc
    ../../src/collector.cc
    ../../src/engine-factory.cc
    ../../src/engines/system-mode-file-format.cc
    ../../src/gcov.cc
    ../../src/output-handler.cc
    ../../src/parser-manager.cc
    ../../src/source-file-cache.cc
    ../../src/system-mode/registration.cc
    ../../src/system-mode/file-data.cc
    ../../src/utils.cc
    ../../src/writers/cobertura-writer.cc
    ../../src/writers/html-writer.cc
    ../../src/writers/writer-base.cc
    main.cc
    tests-collector.cc
    tests-configuration.cc
    tests-elf.cc
    tests-filter.cc
    tests-reporter.cc
    tests-system-mode.cc
    tests-utils.cc
    tests-writer.cc
    )

set (CMAKE_CXX_FLAGS "-std=c++1y -Wall -D_GLIBCXX_USE_NANOSLEEP")


add_custom_command(
   OUTPUT html-data-files.cc
   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bin-to-c-source.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/bcov.css css_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/amber.png icon_amber
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/glass.png icon_glass
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/source-file.html source_file_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/index.html index_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/js/handlebars.js handlebars_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/js/kcov.js kcov_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/js/jquery.min.js jquery_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/js/jquery.tablesorter.min.js tablesorter_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/js/jquery.tablesorter.widgets.min.js tablesorter_widgets_text
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/tablesorter-theme.css tablesorter_theme_text
   > html-data-files.cc
   DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/bcov.css
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/amber.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/glass.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/source-file.html
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/js/handlebars.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/js/kcov.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/js/jquery.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/js/jquery.tablesorter.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/js/jquery.tablesorter.widgets.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/../../data/tablesorter-theme.css
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bin-to-c-source.py
   )

include_directories(
	../../src/include/
	trompeloeil/include
	${LIBCRPCUT_INCLUDE_DIRS}
	${LIBELF_INCLUDE_DIRS}
	${LIBDW_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIR}
	)

add_executable (${TGT} ${${TGT}_SRCS} html-data-files.cc)

target_link_libraries(${TGT}
    ${CMAKE_THREAD_LIBS_INIT}
	"${DL_LIBRARY}"
	${LIBCRPCUT_LIBRARIES}
	${LIBDW_LIBRARIES}
	${LIBELF_LIBRARIES}
	${ZLIB_LIBRARIES}
)

add_executable (same-name-test
	../main.cc
	../subdir/file.c
	../subdir2/file.c
)

add_executable (test-binary test-source.c second-source.c)
SET_TARGET_PROPERTIES(test-binary PROPERTIES COMPILE_FLAGS "-nostdinc")
SET_TARGET_PROPERTIES(test-binary PROPERTIES LINK_FLAGS "-nostdlib")
