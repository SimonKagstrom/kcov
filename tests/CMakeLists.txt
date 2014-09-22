cmake_minimum_required (VERSION 2.6)

project (kcov-tests)

enable_language(ASM)

include_directories(
	${CMAKE_BINARY_DIR}
	../src/include
)
set (CMAKE_CXX_FLAGS "-std=c++0x -Wall -D_GLIBCXX_USE_NANOSLEEP -DKCOV_LIBRARY_PREFIX=${KCOV_LIBRARY_PREFIX}")

add_custom_command (OUTPUT multi-fork-generated.c
  COMMAND ${CMAKE_SOURCE_DIR}/multi-fork/generate-functions.py ${CMAKE_SOURCE_DIR}/multi-fork/code-template.c 1024 > ${CMAKE_BINARY_DIR}/multi-fork-generated.c
  DEPENDS
	${CMAKE_SOURCE_DIR}/multi-fork/generate-functions.py
)

add_custom_target(multi-fork-generated ALL
	DEPENDS multi-fork-generated.c
)
set_property(SOURCE multi-fork/test-multi-fork.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/multi-fork-generated.c)


set (main_tests_SRCS
	main.cc
	subdir/file.c
	subdir2/file.c
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

add_library(shared_library SHARED shared-library/solib.c)


set(CMAKE_BUILD_TYPE distribution)
set(CMAKE_C_FLAGS_DISTRIBUTION "-g")
set(CMAKE_CXX_FLAGS_DISTRIBUTION "-g")


find_library(shared_library_LIBRARY
  NAMES shared_library
  PATHS ${CMAKE_CURRENT_BINARY_DIR}
)


add_executable(main-tests ${main_tests_SRCS})
add_executable(recursive-ptrace ${recursive-ptrace_SRCS})
add_executable(fork ${fork_SRCS})
add_executable(fork_no_wait ${fork_no_wait_SRCS})
add_executable(signals ${signals_SRCS})
add_executable(multi_fork ${multi_fork_SRCS})
add_executable(shared_library_test ${shared_library_test_SRCS})
add_executable(argv_dependent ${argv_dependent_SRCS})
add_executable(test_popen ${test_popen_SRCS})
add_executable(global-constructors ${global_constructors_SRCS})
add_executable(test_daemon ${daemon_SRCS})
add_executable(multi_1 merge-tests/file.c merge-tests/main_1.c)
add_executable(multi_2 merge-tests/file.c merge-tests/main_2.c)
add_executable(setpgid-kill setpgid-kill/setpgid-kill-main.cc ../src/utils.cc)
add_executable(issue31 daemon/test-issue31.cc)


add_custom_target (illegal-insn ALL
  COMMAND ${CMAKE_C_COMPILER} -nostdlib ${CMAKE_CURRENT_SOURCE_DIR}/assembly/illegal-insn.S -o ${CMAKE_BINARY_DIR}/illegal-insn
)

add_custom_target (fork-32 ALL
  COMMAND ${CMAKE_C_COMPILER} -g -m32 ${CMAKE_CURRENT_SOURCE_DIR}/fork/fork.c -o ${CMAKE_BINARY_DIR}/fork-32
)

target_link_libraries(shared_library_test shared_library)
target_link_libraries(global-constructors shared_library)

target_link_libraries(multi_fork
	m)
target_link_libraries(setpgid-kill
	z)
target_link_libraries(issue31
	pthread)


add_custom_target(tests-stripped ALL
	COMMAND strip -o tests-stripped main-tests)
