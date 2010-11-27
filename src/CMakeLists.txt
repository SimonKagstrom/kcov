set (kcov_SRCS
    arch/arch.c
    arch/i386.c
    arch/mips32.c
    arch/x86_64.c
    kc.c
    kc_addr.c
    kc_elf.c
    kc_file.c
    kc_line.c
    kc_ptrace.c
    kprobe_coverage.c
    main.c
    report.c
    utils.c)
include_directories (include/)
include_directories (${GLIB2_INCLUDE_DIRS})
include_directories (${LIBELF_INCLUDE_DIRS})
include_directories (${LIBDWARF_INCLUDE_DIRS})

add_executable (${PROJECT_NAME} ${kcov_SRCS})

target_link_libraries (${PROJECT_NAME} ${GLIB2_LIBRARIES})
target_link_libraries (${PROJECT_NAME} ${LIBELF_LIBRARIES})
target_link_libraries (${PROJECT_NAME} ${LIBDWARF_LIBRARIES})
target_link_libraries (${PROJECT_NAME} pthread)

install (TARGETS ${PROJECT_NAME} ${INSTALL_TARGETS_DEFAULT_ARGS})
