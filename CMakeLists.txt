cmake_minimum_required(VERSION 3.25)

project(lidar_configuration)

if(MSVC)
    add_compile_definitions(_MBCS)
    add_compile_definitions(NOMINMAX)
endif()

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt")
	add_subdirectory(src)
else()
	message(FATAL_ERROR "There is no src/CMakeLists.txt")
endif()
