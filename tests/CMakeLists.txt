cmake_minimum_required (VERSION 2.6)

project (kcov-tests)

set (tests_SRCS
	main.cc
	subdir/file.c
	subdir2/file.c
)

set (fork_SRCS
	fork/fork.c
)

set (fork_no_wait_SRCS
	fork/fork-no-wait.c
)

set (recursive-ptrace_SRCS
	recursive-ptrace/main.cc
)

set(CMAKE_BUILD_TYPE distribution)
set(CMAKE_C_FLAGS_DISTRIBUTION "-g")
set(CMAKE_CXX_FLAGS_DISTRIBUTION "-g")

add_executable(tests ${tests_SRCS})
add_executable(recursive-ptrace ${recursive-ptrace_SRCS})
add_executable(fork ${fork_SRCS})
add_executable(fork_no_wait ${fork_no_wait_SRCS})



add_custom_target(tests-stripped ALL
	COMMAND strip -o tests-stripped tests)

add_custom_target(run-all.sh ALL
	COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/run-all.sh ${CMAKE_CURRENT_BINARY_DIR})
