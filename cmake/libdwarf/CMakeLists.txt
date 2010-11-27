cmake_minimum_required (VERSION 2.8)
set (test_libdwarf_SRCS
     test_libdwarf.c)


add_executable (test_libdwarf ${test_libdwarf_SRCS})
if (ELF_FOUND)
    target_link_libraries (test_libdwarf ${ELF_LIBRARIES})
else (ELF_FOUND)
    target_link_libraries (test_libdwarf elf)
endif (ELF_FOUND)
target_link_libraries (test_libdwarf dwarf)
