cmake_minimum_required (VERSION 2.8)
set (test_libelf_SRCS
     test_libelf.c)

add_executable (test_libelf ${test_libelf_SRCS})
target_link_libraries (test_libelf elf)
