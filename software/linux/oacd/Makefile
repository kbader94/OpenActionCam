# Makefile for building and installing oacd watchdog daemon

CC = gcc
CFLAGS = -Wall
LDFLAGS = -lsystemd
TARGET = oacd
SRC = oacd.c
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SYSTEMD_DIR = /etc/systemd/system

SERVICE_FILE = oacd.service

# Check for sd-daemon.h, warn if not found.
SD_HEADER = /usr/include/systemd/sd-daemon.h

ifeq (,$(wildcard $(SD_HEADER)))
$(error "Missing libsystemd-dev: please run 'sudo apt install libsystemd-dev'")
endif

.PHONY: all install uninstall clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

install: $(TARGET)
	@echo "[*] Installing binary to $(BINDIR)/$(TARGET)"
	sudo install -Dm755 $(TARGET) $(BINDIR)/$(TARGET)

	@echo "[*] Installing systemd service to $(SYSTEMD_DIR)/$(SERVICE_FILE)"
	sudo install -Dm644 $(SERVICE_FILE) $(SYSTEMD_DIR)/$(SERVICE_FILE)

	@echo "[*] Reloading systemd daemon"
	sudo systemctl daemon-reexec

	@echo "[*] Enabling service to start on boot"
	sudo systemctl enable $(SERVICE_FILE)

	@echo "[*] Starting service"
	sudo systemctl start $(SERVICE_FILE)

uninstall:
	@echo "[*] Stopping and disabling service"
	sudo systemctl stop $(SERVICE_FILE) || true
	sudo systemctl disable $(SERVICE_FILE) || true

	@echo "[*] Removing binary and service"
	sudo rm -f $(BINDIR)/$(TARGET)
	sudo rm -f $(SYSTEMD_DIR)/$(SERVICE_FILE)

	@echo "[*] Reloading systemd daemon"
	sudo systemctl daemon-reexec

clean:
	@echo "[*] Cleaning build files"
	rm -f $(TARGET)
