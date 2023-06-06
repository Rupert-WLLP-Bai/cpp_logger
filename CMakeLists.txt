cmake_minimum_required(VERSION 3.15)

project(cpp_logger)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# 输出Boost库的版本号
message(STATUS "Boost version: ${Boost_VERSION}")

# 输出Boost库的路径
message(STATUS "Boost include path: ${Boost_INCLUDE_DIRS}")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

add_executable(cpp_logger ${SOURCES} ${HEADERS})

target_link_libraries(cpp_logger ${Boost_LIBRARIES})
