# === Detect Cross-Compilation Target ===
ARCH ?= x86_64  # Default to compiling for the local system

ifeq ($(ARCH), arm64)
    CC = aarch64-linux-gnu-gcc
    CXX = aarch64-linux-gnu-g++
    TARGET_NAME = linux_camera
else
    CC = gcc
    CXX = g++
    TARGET_NAME = linux_camera
endif

# === Debug/Release Flags ===
DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS = -Wall -g -O0 -DDEBUG -I.
else
    CFLAGS = -Wall -O2 -I.
endif

LDFLAGS = -lpthread

# === Source and Output Files ===
BUILD_DIR = build
SRCS = main.c comms.c record.c
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/$(TARGET_NAME)

# Ensure build directory exists
$(shell mkdir -p $(BUILD_DIR))

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

