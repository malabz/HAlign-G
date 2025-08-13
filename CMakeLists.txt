cmake_minimum_required(VERSION 3.15)
project(MyMainProject)

# 可以统一设置 C++ 标准（如果所有子项目都适用）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")


# 包含子项目
add_subdirectory(halign-mum)
add_subdirectory(halign-g)
add_subdirectory(halign-g2)

