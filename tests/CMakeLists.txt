cmake_minimum_required (VERSION 2.8.4)
project (kcov-tests)

enable_language(ASM)

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

set (CMAKE_THREAD_PREFER_PTHREAD ON)
find_package (Threads REQUIRED)

find_package (ZLIB REQUIRED)

if (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
set (DL_LIBRARY "")
else (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
find_library (DL_LIBRARY NAMES dl)
if (DL_LIBRARY)
    message (STATUS "Found DL: ${DL_LIBRARY}")
else (DL_LIBRARY)
    message (FATAL_ERROR "Could NOT find DL")
endif (DL_LIBRARY)
endif (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)

find_library (M_LIBRARY NAMES m)
if (M_LIBRARY)
    message (STATUS "Found M: ${M_LIBRARY}")
else (M_LIBRARY)
    message (FATAL_ERROR "Could NOT find M")
endif (M_LIBRARY)

include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}"
    ../src/include
)
set (CMAKE_CXX_FLAGS "-std=c++0x -Wall -D_GLIBCXX_USE_NANOSLEEP -DKCOV_LIBRARY_PREFIX=${KCOV_LIBRARY_PREFIX}")

add_custom_command (OUTPUT multi-fork-generated.c
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/multi-fork/generate-functions.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/multi-fork/code-template.c"
            1024
        > "${CMAKE_CURRENT_BINARY_DIR}/multi-fork-generated.c"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/multi-fork/generate-functions.py"
)

add_custom_target(multi-fork-generated ALL
    DEPENDS multi-fork-generated.c
)
set_property(SOURCE multi-fork/test-multi-fork.c APPEND PROPERTY
    OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/multi-fork-generated.c"
)


set (main_tests_SRCS
    main.cc
    subdir/file.c
    subdir2/file2.c
)

set (argv_dependent_SRCS
    argv-dependent.c
)

set (fork_SRCS
    fork/fork.c
)

set (fork_no_wait_SRCS
    fork/fork-no-wait.c
)

set (signals_SRCS
    signals/test-signals.c
)

set (multi_fork_SRCS
    multi-fork/test-multi-fork.c
)

set (recursive-ptrace_SRCS
    recursive-ptrace/main.cc
)

set (shared_library_test_SRCS
    shared-library/main.c
)

set (test_popen_SRCS
    popen/test-popen.c
)

set (global_constructors_SRCS
    global-constructors/test-global-ctors.cc
)

set (daemon_SRCS
    daemon/test-daemon.cc
)

add_library(shared_library SHARED
    shared-library/solib.c
)

add_executable(main-tests ${main_tests_SRCS})
add_executable(fork ${fork_SRCS})
add_executable(fork_no_wait ${fork_no_wait_SRCS})
add_executable(vfork fork/vfork.c)
add_executable(signals ${signals_SRCS})

add_executable(multi_fork ${multi_fork_SRCS})
target_link_libraries(multi_fork "${M_LIBRARY}")

add_executable(shared_library_test ${shared_library_test_SRCS})
target_link_libraries(shared_library_test shared_library)

add_executable(argv_dependent ${argv_dependent_SRCS})
add_executable(test_popen ${test_popen_SRCS})

add_executable(global-constructors ${global_constructors_SRCS})
target_link_libraries(global-constructors shared_library)

add_executable(test_daemon ${daemon_SRCS})
add_executable(multi_1 merge-tests/file.c merge-tests/main_1.c)
add_executable(multi_2 merge-tests/file.c merge-tests/main_2.c)

add_executable(setpgid-kill setpgid-kill/setpgid-kill-main.cc ../src/utils.cc)
target_link_libraries(setpgid-kill ${ZLIB_LIBRARIES})

add_executable(issue31 daemon/test-issue31.cc)
target_link_libraries(issue31 ${CMAKE_THREAD_LIBS_INIT})

add_executable(dlopen dlopen/dlopen.cc dlopen/dlopen-main.cc)
target_link_libraries(dlopen "${DL_LIBRARY}")

add_executable(s short-file.c)
add_executable(fork+exec fork/fork+exec.c)

add_executable(thread-test threads/thread-main.c)
target_link_libraries(thread-test ${CMAKE_THREAD_LIBS_INIT})

#if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
#    add_executable(sanitizer-coverage sanitizer-coverage.c)
#    set_target_properties(sanitizer-coverage PROPERTIES COMPILE_FLAGS "-g -fsanitize=address -fsanitize-coverage=bb")
#    set_target_properties(sanitizer-coverage PROPERTIES LINK_FLAGS "-fsanitize=address -fsanitize-coverage=bb")
#endif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

add_executable (pie pie.c)
if (CMAKE_VERSION VERSION_LESS "2.8.9")
    set_target_properties (pie PROPERTIES COMPILE_FLAGS -fPIE)
else (CMAKE_VERSION VERSION_LESS "2.8.9")
    set_target_properties (pie PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif (CMAKE_VERSION VERSION_LESS "2.8.9")

add_executable (pie-test argv-dependent.c)
if (CMAKE_VERSION VERSION_LESS "2.8.9")
    set_target_properties (pie-test PROPERTIES COMPILE_FLAGS -fPIE)
else (CMAKE_VERSION VERSION_LESS "2.8.9")
    set_target_properties (pie-test PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif (CMAKE_VERSION VERSION_LESS "2.8.9")

if (NOT(CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
    add_executable(recursive-ptrace ${recursive-ptrace_SRCS})
    add_custom_target (illegal-insn ALL
        COMMAND "${CMAKE_C_COMPILER}"
            -nostdlib
            "${CMAKE_CURRENT_SOURCE_DIR}/assembly/illegal-insn.S"
            -o "${CMAKE_CURRENT_BINARY_DIR}/illegal-insn"
    )

    add_custom_target (fork-32 ALL
        COMMAND "${CMAKE_C_COMPILER}" -g -m32
            "${CMAKE_CURRENT_SOURCE_DIR}/fork/fork.c"
            -o "${CMAKE_CURRENT_BINARY_DIR}/fork-32"
    )
endif (NOT(CMAKE_SYSTEM_NAME STREQUAL "Darwin"))

add_custom_target(tests-stripped ALL
    COMMAND "${CMAKE_STRIP}" -o tests-stripped main-tests
    DEPENDS main-tests)
