
set(COMMON_SOURCES
    instructions.cpp
    instructions.h
    rounding_modes.h
)

add_library(common STATIC ${COMMON_SOURCES})

# REMOVE THIS LINE temporarily if backend is empty:
# target_link_libraries(frontend PUBLIC backend)

# Keep only Qt:
target_link_libraries(common PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
