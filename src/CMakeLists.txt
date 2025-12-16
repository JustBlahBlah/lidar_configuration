cmake_minimum_required(VERSION 3.25)

project(lidar_configuration_src)

add_executable(config)

target_sources(config PRIVATE src/main.cpp)

if(WIN32)
    message(STATUS "Building for Windows...")
    target_sources(config PRIVATE src/lidar_helper_windows.cpp)
else()
    message(STATUS "Building for Linux...")
    target_sources(config PRIVATE src/lidar_helper_linux.cpp)
endif()

target_include_directories(config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
