# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\risc-simulator_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\risc-simulator_autogen.dir\\ParseCache.txt"
  "risc-simulator_autogen"
  )
endif()
