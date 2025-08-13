CC = gcc
CXX = g++-9
CFLAGS = -c
CPPFLAGS = -std=c++17
LDFLAGS = -lpthread

# 源文件列表
SRCS_C = multiz/*.c
SRCS_CPP = maf-filter.cpp

# 目标文件列表
OBJS_C = $(patsubst %.c, %.o, $(wildcard $(SRCS_C)))
OBJS_CPP = $(patsubst %.cpp, %.o, $(wildcard $(SRCS_CPP)))

# 生成可执行文件
TARGET = maf-filter

all: $(TARGET)

$(TARGET): $(OBJS_C) $(OBJS_CPP)
	$(CXX) $^ -o $@ -static-libstdc++ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS_C) $(OBJS_CPP)

.PHONY: all
all: | clean $(TARGET)
