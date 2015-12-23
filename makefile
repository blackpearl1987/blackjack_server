CC=gcc
CFLAGS=-o
BUILD_DIR=build
SRC=src

blackjack: clean
	mkdir -p $(BUILD_DIR) 
	$(CC) $(CFLAGS) $(BUILD_DIR)/blackjack $(SRC)/blackjack.c

clean:
	rm -rf build

run_server:
	$(BUILD_DIR)/blackjack
