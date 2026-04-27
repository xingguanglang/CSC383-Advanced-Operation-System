# ============================================================
# Makefile - COEN 383 Project-4 Page Replacement Simulator
# Maintainer: Member 1 (Team Lead)
#
# Usage:
#   make            -> 编译生成 ./pagesim
#   make run        -> 编译后用固定种子运行
#   make clean      -> 清理
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
LDFLAGS =

OBJS = main.o workload.o page_table.o simulator.o stats.o \
       fifo_random.o lru.o lfu_mfu.o

TARGET = pagesim

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Header 依赖
main.o:        common.h workload.h page_table.h simulator.h stats.h
workload.o:    common.h workload.h
page_table.o:  common.h page_table.h
simulator.o:   common.h simulator.h page_table.h replacement.h stats.h
stats.o:       common.h stats.h
fifo_random.o: common.h page_table.h replacement.h
lru.o:         common.h page_table.h replacement.h
lfu_mfu.o:     common.h page_table.h replacement.h

run: $(TARGET)
	./$(TARGET) 42

clean:
	rm -f $(OBJS) $(TARGET) output.txt

.PHONY: all run clean
