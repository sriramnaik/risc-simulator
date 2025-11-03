# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\risc-simulator_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\risc-simulator_autogen.dir\\ParseCache.txt"
  "backend\\CMakeFiles\\backend_autogen.dir\\AutogenUsed.txt"
  "backend\\CMakeFiles\\backend_autogen.dir\\ParseCache.txt"
  "backend\\backend_autogen"
  "backend\\common\\CMakeFiles\\common_autogen.dir\\AutogenUsed.txt"
  "backend\\common\\CMakeFiles\\common_autogen.dir\\ParseCache.txt"
  "backend\\common\\common_autogen"
  "backend\\vm\\CMakeFiles\\vm_autogen.dir\\AutogenUsed.txt"
  "backend\\vm\\CMakeFiles\\vm_autogen.dir\\ParseCache.txt"
  "backend\\vm\\cache\\CMakeFiles\\cache_autogen.dir\\AutogenUsed.txt"
  "backend\\vm\\cache\\CMakeFiles\\cache_autogen.dir\\ParseCache.txt"
  "backend\\vm\\cache\\cache_autogen"
  "backend\\vm\\rv5s\\CMakeFiles\\rv5s_autogen.dir\\AutogenUsed.txt"
  "backend\\vm\\rv5s\\CMakeFiles\\rv5s_autogen.dir\\ParseCache.txt"
  "backend\\vm\\rv5s\\rv5s_autogen"
  "backend\\vm\\vm_autogen"
  "frontend\\CMakeFiles\\frontend_autogen.dir\\AutogenUsed.txt"
  "frontend\\CMakeFiles\\frontend_autogen.dir\\ParseCache.txt"
  "frontend\\frontend_autogen"
  "risc-simulator_autogen"
  )
endif()
