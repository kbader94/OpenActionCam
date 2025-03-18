# === Detect Cross-Compilation Target ===
ARCH ?= x86_64  # Default to compiling for the local system
# Use make ARCH=arm64 to cross-compile

ifeq ($(ARCH), arm64)
    CC = aarch64-linux-gnu-gcc
    CXX = aarch64-linux-gnu-g++
    TARGET_NAME = linux_camera
else
    CC = gcc
    CXX = g++
    TARGET_NAME = linux_camera
endif

CFLAGS = -Wall -g -I.  # Enable debugging & include shared headers
LDFLAGS = -lpthread    # Link additional libraries if needed

# Directories
BUILD_DIR = build
SRCS = main.c comms.c record.c  # No serial_wrapper.cpp for Linux
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/$(TARGET_NAME)

# Ensure build directory exists
$(shell mkdir -p $(BUILD_DIR))

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

debug: CFLAGS += -DDEBUG
debug: all

