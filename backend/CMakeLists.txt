set(BACKEND_SOURCES
    command_handler.cpp
    command_handler.h
    config.cpp
    config.h
    globals.cpp
    globals.h
    mainloop.cpp
    mainloop.h
    utils.cpp
    utils.h
    vm_asm_mw.cpp
    vm_asm_mw.h
    vm_runner.cpp
    vm_runner.h

    # Assembler sources
    assembler/assembler.cpp
    assembler/assembler.h
    assembler/code_generator.cpp
    assembler/code_generator.h
    assembler/elf_util.cpp
    assembler/elf_util.h
    assembler/errors.cpp
    assembler/errors.h
    assembler/lexer.cpp
    assembler/lexer.h
    assembler/parser.cpp
    assembler/parser.h
    assembler/tokens.cpp
    assembler/tokens.h

    # Parse formats
    assembler/parse_formats/csr_formats.cpp
    assembler/parse_formats/f_d_formats.cpp
    assembler/parse_formats/i_m_formats.cpp
    assembler/parse_formats/pseudo_formats.cpp
)

# Add subdirectories for common and vm
add_subdirectory(common)
add_subdirectory(vm)

# Create ONE big backend library
add_library(backend STATIC ${BACKEND_SOURCES})

target_link_libraries(backend PUBLIC
    common
    vm
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/assembler
    ${CMAKE_CURRENT_SOURCE_DIR}/assembler/parse_formats
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/vm
)
