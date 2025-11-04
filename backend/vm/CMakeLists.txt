# add_subdirectory(cache)
# add_subdirectory(rv5s)
# add_subdirectory(rvss)

# set(VM_SOURCES
#     alu.cpp
#     alu.h
#     control_unit_base.cpp
#     control_unit_base.h
#     main_memory.cpp
#     main_memory.h
#     memory_controller.cpp
#     memory_controller.h
#     mmio_devices.h
#     registers.cpp
#     registers.h
#     vm_base.cpp
#     vm_base.h
# )

# add_library(vm STATIC ${VM_SOURCES})

# # REMOVE THIS LINE temporarily if backend is empty:
# # target_link_libraries(frontend PUBLIC backend)

# # Keep only Qt:
# target_link_libraries(vm PUBLIC
#     Qt${QT_VERSION_MAJOR}::Widgets
# )

# target_include_directories(vm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# # Add subdirectories first
# add_subdirectory(cache)
# add_subdirectory(rv5s)
# add_subdirectory(rvss)

# set(VM_SOURCES
#     alu.cpp
#     alu.h
#     control_unit_base.cpp
#     control_unit_base.h
#     main_memory.cpp
#     main_memory.h
#     memory_controller.cpp
#     memory_controller.h
#     mmio_devices.h
#     registers.cpp
#     registers.h
#     vm_base.cpp
#     vm_base.h
# )

# add_library(vm STATIC ${VM_SOURCES})

# target_link_libraries(vm PUBLIC
#     cache
#     rv5s
#     rvss
#     common  # Likely needed for instructions
#     Qt${QT_VERSION_MAJOR}::Widgets
# )

# target_include_directories(vm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Add subdirectories FIRST
add_subdirectory(cache)
add_subdirectory(rv5s)
# add_subdirectory(rvss)

set(VM_SOURCES
    alu.cpp
    alu.h
    control_unit_base.cpp
    control_unit_base.h
    main_memory.cpp
    main_memory.h
    memory_controller.cpp
    memory_controller.h
    mmio_devices.h
    registers.cpp
    registers.h
    vm_base.cpp
    vm_base.h

    rvss_vm.cpp
    rvss_vm.h
    rvss_control_unit.cpp
    rvss_control_unit.h
)

add_library(vm STATIC ${VM_SOURCES}
    hazardUnit.h
    pipelineRegisters.h
    rvss_vm_pipelined.h rvss_vm_pipelined.cpp
    hazardUnit.cpp
    forwarding_unit.h forwarding_unit.cpp)

# vm needs to link its subdirectories AND common
target_link_libraries(vm PUBLIC
    cache
    rv5s
    # rvss
    common
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(vm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
