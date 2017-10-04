BUILDROOT ?= ${PWD}/../buildroot
KERNEL_VERSION ?= linux-2017.2_video_ea
ARCH ?= arm64
CROSS_COMPILE := $(BUILDROOT)/output/host/bin/aarch64-linux-gnu-
KERNEL_DIR ?= $(BUILDROOT)/output/build/$(KERNEL_VERSION)

.PHONY: all modules modules_clean

all: modules
clean: modules_clean

modules:
	ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(KERNEL_DIR) M=$(CURDIR)

modules_clean:
	ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean
