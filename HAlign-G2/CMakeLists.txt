cmake_minimum_required(VERSION 3.8)

project("halign-g2")

set(CMAKE_CXX_COMPILER g++)

# ====== 公共设置 ======
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ====== halign-g2 部分 ======

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 源文件列表
file(GLOB_RECURSE SRCS_C
    "multiz/*.c"
)
file(GLOB_RECURSE SRCS_CPP
    "PairwiseAlignment/*.cpp"
    "SuffixArray/*.cpp"
    "Utils/*.cpp"
    "multi-thread/*.cpp"
    "ztmsa.cpp"
)

# 添加可执行文件
add_executable(halign-g2 ${SRCS_C} ${SRCS_CPP})

# 链接库
target_link_libraries(halign-g2 pthread)

# ====== mynuc 部分 ======

# 编译选项（静态编译）
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pthread -static")

# 源码目录
set(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})

# 递归查找所有 .cpp 和 .cc 文件
file(GLOB_RECURSE MYNUC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/nucmer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/nucmer/*.cc"
)

# 添加可执行文件
add_executable(mynuc ${MYNUC_SOURCES})

target_compile_options(mynuc PRIVATE -O3 -pthread -static)
