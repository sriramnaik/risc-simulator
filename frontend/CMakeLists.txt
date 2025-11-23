# frontend/CMakeLists.txt

# All UI-related source files
set(FRONTEND_SOURCES
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    codeEditor.h
    codeEditor.cpp
    RegisterTable.h
    RegisterTable.cpp
    registerpanel.h
    registerpanel.cpp
    controlstatustable.h
    controlstatustable.cpp
    errorconsole.h
    errorconsole.cpp
    bottompanel.cpp
    bottompanel.h
    datasegment.h
    datasegment.cpp
)

# Create frontend static library
add_library(frontend STATIC ${FRONTEND_SOURCES}
    processorwindow.h processorwindow.cpp
    VMExecutionThread.h
    VMExecutionThread.h
    VMExecutionThread.cpp
    InstructionInfo.h
    InstructionInfo.cpp
    autocompletewidget.h autocompletewidget.cpp

)

# Link Qt Widgets and backend
target_link_libraries(frontend PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
    backend
)

# Make frontend headers accessible to main project
target_include_directories(frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


# frontend/CMakeLists.txt

# set(FRONTEND_SOURCES
#     mainwindow.cpp
#     mainwindow.h
#     mainwindow.ui
#     codeEditor.h
#     codeEditor.cpp
#     RegisterTable.h
#     RegisterTable.cpp
#     registerpanel.h
#     registerpanel.cpp
#     controlstatustable.h
#     controlstatustable.cpp
#     errorconsole.h
#     errorconsole.cpp
#     bottompanel.cpp
#     bottompanel.h
#     datasegment.h
#     datasegment.cpp
# )

# add_library(frontend STATIC ${FRONTEND_SOURCES})

# # REMOVE THIS LINE temporarily if backend is empty:
# # target_link_libraries(frontend PUBLIC backend)

# # Keep only Qt:
# target_link_libraries(frontend PUBLIC
#     Qt${QT_VERSION_MAJOR}::Widgets
# )

# target_include_directories(frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
