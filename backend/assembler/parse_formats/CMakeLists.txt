
set(PARSERFORMAT_SOURCES
    csr_formats.cpp
    f_d_formats.cpp
    i_m_formats.cpp
    pseudo_formats.cpp
)

add_library(parser_format STATIC ${PARSERFORMAT_SOURCES})

# REMOVE THIS LINE temporarily if backend is empty:
# target_link_libraries(frontend PUBLIC backend)

# Keep only Qt:
target_link_libraries(parser_format PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(parser_format PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
