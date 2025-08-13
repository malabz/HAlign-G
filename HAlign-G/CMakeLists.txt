cmake_minimum_required(VERSION 3.12)
project(halign-g LANGUAGES C CXX)

find_program(GPP9_EXISTS g++-9)
if (GPP9_EXISTS)
  set(CMAKE_CXX_COMPILER ${GPP9_EXISTS})
else()
  find_program(DEFAULT_GPP g++)
  set(CMAKE_CXX_COMPILER ${DEFAULT_GPP})
endif()

# 设置 C 和 C++ 编译选项
# set(CMAKE_C_FLAGS "-c")
set(CMAKE_CXX_FLAGS " -std=c++17")

# 源文件
file(GLOB SRCS_C "src/multiz/*.c")
set(SRCS_CPP
  src/PairwiseAlignment/NeedlemanWunshReusable.cpp
  src/SuffixArray/parallel_import.cpp
  src/Utils/MurmurHash3.cpp
  src/Utils/Arguments.cpp
  src/Utils/Fasta.cpp
  src/Utils/Insertion.cpp
  src/Utils/Utils.cpp
  src/StarAlignment/StarAligner.cpp
  src/multi-thread/multi.cpp
  src/ztmsa.cpp
)

add_executable(${PROJECT_NAME} ${SRCS_C} ${SRCS_CPP})

# 静态链接 pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} Threads::Threads)

# 指定使用静态链接（gcc/g++）
if(MSVC)
  # MSVC 平台的话，可以添加类似 /MT 标志实现静态链接 CRT
  target_compile_options(${PROJECT_NAME} PRIVATE /MT)
else()
  # Unix-like 系统加 -static 选项
  target_link_options(${PROJECT_NAME} PRIVATE -static)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<COMPILE_LANGUAGE:C>:-c>
  $<$<COMPILE_LANGUAGE:CXX>:-c -std=c++17>
)
