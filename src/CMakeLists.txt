cmake_minimum_required(VERSION 3.25)

project(lidar_configuration)

add_executable(config)

target_sources(config 
	PRIVATE
		src/main.cpp

	PRIVATE FILE_SET HEADERS
	BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src
	FILES
		include/defines.hpp
		include/lidar_helper.hpp
)
