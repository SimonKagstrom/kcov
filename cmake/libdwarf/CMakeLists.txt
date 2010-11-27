cmake_minimum_required (VERSION 2.8)
set (test_libdwarf_SRCS
     test_libdwarf.c)

include_directories (${LIBELF_INCLUDE_DIRS})
add_executable (test_libdwarf ${test_libdwarf_SRCS})
target_link_libraries (test_libdwarf ${LIBELF_LIBRARIES})
target_link_libraries (test_libdwarf dwarf)
