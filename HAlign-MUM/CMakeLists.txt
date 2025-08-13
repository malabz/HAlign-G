cmake_minimum_required(VERSION 3.15)


project("halign-mum")


find_program(GPP9_EXISTS g++-9)
if (GPP9_EXISTS)
  set(CMAKE_CXX_COMPILER g++-9)
else()
  set(CMAKE_CXX_COMPILER g++)
endif()


# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 源文件列表
file(GLOB_RECURSE SRCS_C "multiz/*.c")
file(GLOB_RECURSE SRCS_CPP "PairwiseAlignment/*.cpp" "SuffixArray/*.cpp" "Utils/*.cpp" "multi-thread/*.cpp" "ztmsa.cpp")

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SRCS_C} ${SRCS_CPP})

# 链接库
target_link_libraries(${PROJECT_NAME} pthread)

target_link_options(${PROJECT_NAME} PRIVATE -static)


set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

# 额外要拷贝的程序路径列表
set(EXTRA_PROGRAMS
    "${PROJECT_SOURCE_DIR}/hal2maf"
    "${PROJECT_SOURCE_DIR}/maf2hal"
    "${PROJECT_SOURCE_DIR}/mafDuplicateFilter"
)


# 固定输出目录，方便管理和清理
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# 拷贝动作，放到同一目录
foreach(PROG ${EXTRA_PROGRAMS})
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      ${PROG}
      "${CMAKE_BINARY_DIR}"
  )
endforeach()

# 4. 配置阶段写死清理路径
set(ADDITIONAL_MAKE_CLEAN_FILES
  "${CMAKE_BINARY_DIR}/hal2maf"
  "${CMAKE_BINARY_DIR}/maf2hal"
  "${CMAKE_BINARY_DIR}/mafDuplicateFilter"
)

set_directory_properties(PROPERTIES
  ADDITIONAL_MAKE_CLEAN_FILES "${ADDITIONAL_MAKE_CLEAN_FILES}"
)

file(GLOB SRCS_MULTIZ_C "multiz_bin/multiz/*.c")

set(SRCS_MULTIZ_CPP
  multiz_bin/PairwiseAlignment/NeedlemanWunshReusable.cpp
  multiz_bin/SuffixArray/parallel_import.cpp
  multiz_bin/Utils/Arguments.cpp
  multiz_bin/Utils/Insertion.cpp
  multiz_bin/Utils/Utils.cpp
  multiz_bin/Utils/mum_main.cpp
  multiz_bin/multi-thread/multi.cpp
  multiz_bin/mymultiz.cpp
)

add_executable(mymultiz ${SRCS_MULTIZ_C} ${SRCS_MULTIZ_CPP})

find_package(Threads REQUIRED)
target_link_libraries(mymultiz Threads::Threads)

if(NOT MSVC)
  # 传递 -static 让链接器尽可能静态链接所有库（libc, pthread 等）
  target_link_options(mymultiz PRIVATE -static -static-libstdc++)
endif()

set_property(TARGET mymultiz PROPERTY CXX_STANDARD 17)
set_property(TARGET mymultiz PROPERTY CXX_STANDARD_REQUIRED ON)

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

