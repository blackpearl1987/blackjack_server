CC=gcc
CFLAGS=-o
BUILD_DIR=build
SRC=src

blackjack: clean pre program1
	

pre:
	mkdir -p $(BUILD_DIR) 

program1:
	$(CC) $(CFLAGS) $(BUILD_DIR)/blackjack $(SRC)/blackjack.c

program2:
	$(CC) $(CFLAGS) $(BUILD_DIR)/blackjack_listener $(SRC)/blackjack_listener.c

clean:
	rm -rf build

run:
	$(BUILD_DIR)/blackjack 8
