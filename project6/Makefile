# Makefile - COEN 383 Project 6
# Person 5: build / test / package
#
# Targets:
#   make         build ./project6
#   make run     run interactively (you type for Child 5)
#   make sample  run with sample_input.txt as Child 5's stdin
#   make clean   remove binaries and output.txt
#   make zip     produce Group-N.zip for submission (edit GROUP below)

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE
TARGET  = project6
OBJS    = main.o child.o parent.o utils.o
HDRS    = utils.h child.h parent.h
GROUP   = Group-N

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

sample: $(TARGET) sample_input.txt
	./$(TARGET) < sample_input.txt

clean:
	rm -f $(TARGET) $(OBJS) output.txt $(GROUP).zip

zip: clean
	zip $(GROUP).zip *.c *.h Makefile sample_input.txt README.md output.txt 2>/dev/null || true

.PHONY: all run sample clean zip
