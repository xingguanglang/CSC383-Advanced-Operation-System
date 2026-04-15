# Makefile - COEN 383 Project 2: Process Scheduling Simulator

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99
TARGET  = scheduler

SRCS    = main.c process.c stats.c fcfs.c sjf.c srt.c rr.c hpf_np.c hpf_p.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c process.h
	$(CC) $(CFLAGS) -c $<

run: $(TARGET)
	./$(TARGET)

output: $(TARGET)
	./$(TARGET) > simulation_output.txt
	@echo "Output saved to simulation_output.txt"

clean:
	rm -f $(OBJS) $(TARGET) simulation_output.txt

.PHONY: all run output clean
