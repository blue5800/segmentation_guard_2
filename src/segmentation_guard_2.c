#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>

#include "bad_area_hook.h"
#include "sg2_control.h"


unsigned long bad_area_nosemaphore_addr, do_mprotect_pkey_addr, do_mmap_addr;

static unsigned long lookup_kallsyms_lookup_name(const char *name) {
	struct kprobe kp;
	unsigned long addr;

	memset(&kp, 0, sizeof(kp));
	kp.symbol_name = name;
	if (register_kprobe(&kp) < 0) {
		return 0;
	}
	addr = (unsigned long)kp.addr;
	unregister_kprobe(&kp);
	return addr;
}

struct kprobe bad_area_nosemaphore_kp = {
	.symbol_name = "__bad_area_nosemaphore",
	.pre_handler = replace_bad_area_nosemaphore,
	.post_handler = boringpost,
};

struct kprobe sys_reboot_kp = {
	.symbol_name = "__do_sys_reboot",
	.pre_handler = control_sg2,
	.post_handler = boringpost,
};

static int tmp(struct kprobe *kp, struct pt_regs *regs) {
	printk(KERN_INFO "Segmentation Guard 2: Process %s (PID %d) is exiting\n", current->comm, current->pid);
	return 0;
}

struct kprobe do_exit_kp = {
	.symbol_name = "do_exit",
	.pre_handler = tmp,
};

do_mprotect_pkey_t do_mprotect_pkey_fn;
do_mmap_t do_mmap_fn;

enum sg2_status current_sg2_status = SG2_STATUS_GLOBAL_ENABLED;

static int segmentation_guard_2_init(void) {
	printk(KERN_INFO "Segmentation Guard 2: Module loaded successfully\n");

	do_mprotect_pkey_addr = lookup_kallsyms_lookup_name("do_mprotect_pkey");
	do_mmap_addr = lookup_kallsyms_lookup_name("do_mmap");

	if (!do_mprotect_pkey_addr || !do_mmap_addr) {
		printk(KERN_ERR "Segmentation Guard 2: Failed to resolve needed symbols\n");
		return -ENOENT;
	}
	printk(KERN_INFO "Segmentation Guard 2: Found do_mprotect_pkey at %lx\nFound do_mmap at %lx\n", do_mprotect_pkey_addr, do_mmap_addr);

	do_mprotect_pkey_fn = (do_mprotect_pkey_t) do_mprotect_pkey_addr;
	do_mmap_fn = (do_mmap_t) do_mmap_addr;

	if (register_kprobe(&bad_area_nosemaphore_kp) < 0) {
		printk(KERN_ERR "Segmentation Guard 2: Failed to register kprobe\n");
		return -EFAULT;
	}

	if (register_kprobe(&sys_reboot_kp) < 0) {
		printk(KERN_ERR "Segmentation Guard 2: Failed to register kprobe\n");
		unregister_kprobe(&bad_area_nosemaphore_kp);
		return -EFAULT;
	}

	if (register_kprobe(&do_exit_kp) < 0) {
		printk(KERN_ERR "Segmentation Guard 2: Failed to register tracepoint\n");
		unregister_kprobe(&bad_area_nosemaphore_kp);
		unregister_kprobe(&sys_reboot_kp);
		return -EFAULT;
	}

	printk(KERN_INFO "Segmentation Guard 2: Kprobe registered successfully\n");
	return 0;
}

static void segmentation_guard_2_exit(void) {
	unregister_kprobe(&bad_area_nosemaphore_kp);
	unregister_kprobe(&sys_reboot_kp);
	unregister_kprobe(&do_exit_kp);
	printk(KERN_INFO "Segmentation Guard 2: Module unloaded successfully\n");
}

MODULE_DESCRIPTION("Segmentation Guard 2: A kernel module to fix segmentation faults");
MODULE_AUTHOR("blue5800");
MODULE_LICENSE("GPL");

module_init(segmentation_guard_2_init);
module_exit(segmentation_guard_2_exit);
