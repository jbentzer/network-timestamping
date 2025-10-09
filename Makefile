# Makefile for the network-timestamps applications

# Compiler and flags
CC = g++
CFLAGS = -Wall -O2 -std=c++17

# Source and output
SRC_RECEIVER = receiver.cpp
OUT_RECEIVER = receiver

SRC_SENDER = sender.cpp
OUT_SENDER = sender

# Default target (executed when you just run `make`)
all: $(OUT_RECEIVER) $(OUT_SENDER)

# build receiver
$(OUT_RECEIVER): $(SRC_RECEIVER)
	$(CC) $(CFLAGS) -o $(OUT_RECEIVER) $(SRC_RECEIVER)
	@echo "Build of receiver complete!"

# build sender
$(OUT_SENDER): $(SRC_SENDER)
	$(CC) $(CFLAGS) -o $(OUT_SENDER) $(SRC_SENDER)
	@echo "Build of sender complete!"

# Second target: clean up generated files
clean:
	rm -f $(OUT_RECEIVER)
	rm -f $(OUT_SENDER)
	@echo "Cleaned up."
