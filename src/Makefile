CC = gcc
CXX = g++
CFLAGS = -c
CPPFLAGS = -std=c++17
LDFLAGS = -lpthread

# 检查系统中是否存在 g++-9 编译器
GPP9_EXISTS := $(shell command -v g++-9 2> /dev/null)

ifeq ($(strip $(GPP9_EXISTS)),)
  # 如果 g++-9 不存在，则使用默认的 g++ 编译器
  CXX = g++
else
  # 如果 g++-9 存在，则将 CXX 设置为 g++-9
  CXX = g++-9
endif

# 源文件列表
SRCS_C = multiz/*.c
SRCS_CPP = PairwiseAlignment/NeedlemanWunshReusable.cpp SuffixArray/parallel_import.cpp Utils/MurmurHash3.cpp Utils/Arguments.cpp Utils/Fasta.cpp Utils/Insertion.cpp Utils/Utils.cpp StarAlignment/StarAligner.cpp multi-thread/multi.cpp ztmsa.cpp

# 目标文件列表
OBJS_C = $(patsubst %.c, %.o, $(wildcard $(SRCS_C)))
OBJS_CPP = $(patsubst %.cpp, %.o, $(wildcard $(SRCS_CPP)))

# 生成可执行文件
TARGET = halign-g

all: $(TARGET)

$(TARGET): $(OBJS_C) $(OBJS_CPP)
	$(CXX) $^ -o $@  $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS_C) $(OBJS_CPP)

.PHONY: all
all: $(OBJS_C) $(OBJS_CPP)
