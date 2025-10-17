UNAME := $(shell uname)
EXT =

CFLAGS = -Wall -Wextra -I./include
LDFLAGS = -lportaudio -lopus

ifeq ($(UNAME), Darwin)
    CC = clang
    CFLAGS += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib -lpthread
endif

ifeq ($(UNAME), Linux)
    CC = gcc
    LDFLAGS += -lpthread
endif

ifeq ($(findstring MINGW64_NT,$(UNAME)), MINGW64_NT)
    CC = x86_64-w64-mingw32-gcc
	CFLAGS += -D_WINDOWS
    LDFLAGS += -lws2_32
    EXT = .exe
endif

BUILD_DIR = build
SRC_DIR = src

SENDER = $(BUILD_DIR)/sender$(EXT)
RECEIVER = $(BUILD_DIR)/receiver$(EXT)

COMMON_SRC = $(SRC_DIR)/network.c $(SRC_DIR)/audio.c $(SRC_DIR)/ntp.c $(SRC_DIR)/codec.c
SENDER_SRC = $(SRC_DIR)/sender.c $(COMMON_SRC)
RECEIVER_SRC = $(SRC_DIR)/receiver.c $(COMMON_SRC)

all: $(SENDER) $(RECEIVER)

$(SENDER): $(SENDER_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(RECEIVER): $(RECEIVER_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
	rm -f output.raw

.PHONY: all clean
