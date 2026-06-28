ifneq ($(KERNELRELEASE),)
	CONFIG_SEGMENTATION_GUARD_2 ?= m
	obj-$(CONFIG_SEGMENTATION_GUARD_2) += segmentation_guard_2.o
	segmentation_guard_2-y := src/segmentation_guard_2.o src/bad_area_hook.o src/sg2_control.o src/proc_tracker.o
	segmentation_guard_2-$(CONFIG_X86_KERNEL_IBT) += src/ibt_wrappers.o
else
	KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
endif

