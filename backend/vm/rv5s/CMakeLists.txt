
# set(RV5S_SOURCES
#     rv5s_control_unit.cpp
#     rv5s_control_unit.h
#     rv5s_vm.cpp
#     rv5s_vm.h
# )

# add_library(rv5s STATIC ${RV5S_SOURCES})

# # REMOVE THIS LINE temporarily if backend is empty:
# # target_link_libraries(frontend PUBLIC backend)

# # Keep only Qt:
# target_link_libraries(rv5s PUBLIC
#     Qt${QT_VERSION_MAJOR}::Widgets
# )

# target_include_directories(rv5s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(RV5S_SOURCES
    rv5s_control_unit.cpp
    rv5s_control_unit.h
    rv5s_vm.cpp
    rv5s_vm.h
)

add_library(rv5s STATIC ${RV5S_SOURCES})

target_link_libraries(rv5s PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
)

# CRITICAL: Include parent directory to access vm's headers
target_include_directories(rv5s PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # Access parent vm/ directory
)
