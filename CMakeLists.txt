cmake_minimum_required(VERSION 3.25)

project(lidar_configuration)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt")
	add_subdirectory(src)
else()
	message(FATL_ERROR "There is no src/CMakeLists.txt")
endif()

