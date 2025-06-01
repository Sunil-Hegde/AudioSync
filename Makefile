# For macOS
ifeq ($(shell uname), Darwin)
    CC = clang
else
    CC = gcc
endif

CFLAGS = -Wall -Wextra -I./include
BUILD_DIR = build
SRC_DIR = src

SENDER = $(BUILD_DIR)/sender
RECEIVER = $(BUILD_DIR)/receiver

COMMON_SRC = $(SRC_DIR)/network.c $(SRC_DIR)/audio.c
SENDER_SRC = $(SRC_DIR)/sender.c $(COMMON_SRC)
RECEIVER_SRC = $(SRC_DIR)/receiver.c $(COMMON_SRC)

all: $(SENDER) $(RECEIVER)

$(SENDER): $(SENDER_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(RECEIVER): $(RECEIVER_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(SENDER) $(RECEIVER)
	rm output.raw
.PHONY: all clean