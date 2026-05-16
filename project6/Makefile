CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE
TARGET  = project6
SRC     = project6.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

# Run with a canned input file for repeatable testing
sample: $(TARGET) sample_input.txt
	./$(TARGET) < sample_input.txt

clean:
	rm -f $(TARGET) output.txt

.PHONY: all run sample clean
