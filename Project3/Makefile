# =============================================================================
#  COEN 383 Project 3 - Multi-threaded Ticket Sellers
#  Makefile
#
#  Usage:
#    make            # builds the executable
#    make run        # runs N=5, 10, 15 and writes everything to output.txt
#    make run5       # runs only N=5  -> output_N5.txt
#    make run10      # runs only N=10 -> output_N10.txt
#    make run15      # runs only N=15 -> output_N15.txt
#    make clean      # removes the executable and all output*.txt files
# =============================================================================

CC      = gcc
CFLAGS  = -Wall -O2 -pthread
SRC     = project3_ticket_sellers.c
TARGET  = ticket_sim

# Default target: build the executable
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Run all three required N values, append everything into output.txt
run: $(TARGET)
	@echo "Running simulation for N = 5, 10, 15 ..."
	@echo "================================================================" >  output.txt
	@echo " COEN 383 Project 3 - Multi-threaded Ticket Sellers"               >> output.txt
	@echo " Combined simulation output for N = 5, 10, 15"                     >> output.txt
	@echo "================================================================" >> output.txt
	@echo ""                                                                  >> output.txt
	@echo ""                                                                  >> output.txt
	@echo "################################################################" >> output.txt
	@echo "#                    SIMULATION RUN: N = 5                     #" >> output.txt
	@echo "################################################################" >> output.txt
	./$(TARGET) 5  >> output.txt
	@echo ""                                                                  >> output.txt
	@echo ""                                                                  >> output.txt
	@echo "################################################################" >> output.txt
	@echo "#                    SIMULATION RUN: N = 10                    #" >> output.txt
	@echo "################################################################" >> output.txt
	./$(TARGET) 10 >> output.txt
	@echo ""                                                                  >> output.txt
	@echo ""                                                                  >> output.txt
	@echo "################################################################" >> output.txt
	@echo "#                    SIMULATION RUN: N = 15                    #" >> output.txt
	@echo "################################################################" >> output.txt
	./$(TARGET) 15 >> output.txt
	@echo "Done. Results written to output.txt"

# Individual runs (each goes to its own file)
run5: $(TARGET)
	./$(TARGET) 5  > output_N5.txt
	@echo "Done. Results written to output_N5.txt"

run10: $(TARGET)
	./$(TARGET) 10 > output_N10.txt
	@echo "Done. Results written to output_N10.txt"

run15: $(TARGET)
	./$(TARGET) 15 > output_N15.txt
	@echo "Done. Results written to output_N15.txt"

clean:
	rm -f $(TARGET) output.txt output_N5.txt output_N10.txt output_N15.txt

.PHONY: all run run5 run10 run15 clean