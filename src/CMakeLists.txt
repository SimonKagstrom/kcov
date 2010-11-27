set (kcov_SRCS
    arch/arch
    arch/i386
    arch/mips32
    arch/x86_64
    kc
    kc_addr
    kc_elf
    kc_file
    kc_line
    kc_ptrace
    kprobe_coverage
    main
    report
    utils)
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
