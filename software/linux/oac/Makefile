KDIR := /lib/modules/$(shell uname -r)/build
ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-

# Two independent modules
obj-m += oac_driver.o
obj-m += oac_watchdog_driver.o

# Compose each module from source object files
oac_driver-objs := oac_dev.o oac_comms.o
oac_watchdog_driver-objs := oac_watchdog.o

# Device Tree Overlay
DT_SOURCE := oac.dtso
DT_OVERLAY := oac.dtbo
DT_HASH_FILE := .last_dtso.hash

all: modules dtbo

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

dtbo: $(DT_OVERLAY)

%.dtbo: %.dtso
	dtc -@ -I dts -O dtb -o $@ $<

# Install the kernel modules and reboot if necessary
# Note: we hash the DT overlay source to trigger a reboot if the overlay source changes
install: all
	@echo "[*] Installing kernel modules to /lib/modules/$(shell uname -r)/extra"
	sudo mkdir -p /lib/modules/$(shell uname -r)/extra
	sudo cp *.ko /lib/modules/$(shell uname -r)/extra
	sudo depmod

	@echo "[*] Checking for device tree changes..."
	@CURRENT_HASH=$$(sha256sum $(DT_SOURCE) | awk '{ print $$1 }'); \
	PREVIOUS_HASH=$$(cat $(DT_HASH_FILE) 2>/dev/null || echo "none"); \
	if [ "$$CURRENT_HASH" != "$$PREVIOUS_HASH" ]; then \
		echo "  ↪ Device tree changed — installing and rebooting..."; \
		sudo cp $(DT_OVERLAY) /boot/overlays/; \
		echo "$$CURRENT_HASH" > $(DT_HASH_FILE); \
		sudo reboot; \
	else \
		echo "  ↪ No changes to device tree — skipping reboot."; \
	fi

	@echo "[✔] Install complete."

clean:
	@rm -f *.o *.ko .*.mod .*.cmd *.mod.c *.mod.o *.symvers *.order *.dtbo *.dtb $(DT_HASH_FILE)
	@rm -rf .tmp_versions modules.order Module.symvers

.PHONY: all modules dtbo install clean
