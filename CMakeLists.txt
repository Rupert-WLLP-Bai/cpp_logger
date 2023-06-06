cmake_minimum_required(VERSION 3.12)
project(Logger)

set(CMAKE_CXX_STANDARD 14)

# 添加Google Benchmark依赖
find_package(benchmark REQUIRED)

# 添加Google Test依赖
find_package(GTest REQUIRED)

add_executable(cpp_logger main.cpp log.cpp)

# 添加测试执行文件
add_executable(LoggerTest log_test.cpp log.cpp)
target_link_libraries(LoggerTest PRIVATE GTest::GTest GTest::Main)

# 注册 LoggerTest 作为测试
include(GoogleTest)
gtest_discover_tests(LoggerTest)

# 添加 Google Test 相关设置
set_target_properties(LoggerTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
)

target_link_libraries(cpp_logger PRIVATE benchmark::benchmark)
