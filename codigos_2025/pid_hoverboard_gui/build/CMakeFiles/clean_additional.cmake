# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/PID_Hoverboard_GUI_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/PID_Hoverboard_GUI_autogen.dir/ParseCache.txt"
  "PID_Hoverboard_GUI_autogen"
  )
endif()
