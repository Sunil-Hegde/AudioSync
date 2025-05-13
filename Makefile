OS := $(shell uname)
# For Linux
ifeq ($(OS), Linux)
    CC = gcc
endif
# For macOS
ifeq ($(OS), Darwin)
    CC = clang
endif
CFLAGS = -std=c17 -Wall -Wextra -Werror
BUILD_DIR = build
SRC_DIR = src

SENDER = $(BUILD_DIR)/sender
RECEIVER = $(BUILD_DIR)/receiver

COMMON_SRC = $(SRC_DIR)/network.c
SENDER_SRC = $(SRC_DIR)/sender.c $(COMMON_SRC)
RECEIVER_SRC = $(SRC_DIR)/receiver.c $(COMMON_SRC)

all: $(SENDER) $(RECEIVER)

$(SENDER): $(SENDER_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I. $^ -o $@

$(RECEIVER): $(RECEIVER_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I. $^ -o $@

clean:
	rm -f $(SENDER) $(RECEIVER)
