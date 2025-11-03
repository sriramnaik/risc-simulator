
# set(CACHE_SOURCES
#     cache.cpp
#     cache.h
# )

# add_library(cache STATIC ${CACHE_SOURCES})

# # REMOVE THIS LINE temporarily if backend is empty:
# # target_link_libraries(frontend PUBLIC backend)

# # Keep only Qt:
# target_link_libraries(cache PUBLIC
#     Qt${QT_VERSION_MAJOR}::Widgets
# )

# target_include_directories(cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(CACHE_SOURCES
    cache.cpp
    cache.h
)

add_library(cache STATIC ${CACHE_SOURCES})

target_link_libraries(cache PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Include parent directory to access vm's headers if needed
target_include_directories(cache PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..  # Access parent vm/ directory
)
