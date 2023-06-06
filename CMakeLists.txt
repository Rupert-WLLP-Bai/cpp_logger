cmake_minimum_required(VERSION 3.12)
project(Logger)

set(CMAKE_CXX_STANDARD 14)

# 添加Google Benchmark依赖
find_package(benchmark REQUIRED)

add_executable(cpp_logger main.cpp log.cpp)
target_link_libraries(cpp_logger PRIVATE benchmark::benchmark)
