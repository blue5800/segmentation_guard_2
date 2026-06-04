#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include "bad_area_hook.h"

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

do_mprotect_pkey_t do_mprotect_pkey_fn;
do_mmap_t do_mmap_fn;

static int segmentation_guard_2_init(void) {
	printk(KERN_INFO "Segmentation Guard 2: Module loaded successfully\n");

	bad_area_nosemaphore_addr = lookup_kallsyms_lookup_name("__bad_area_nosemaphore");
	do_mprotect_pkey_addr = lookup_kallsyms_lookup_name("do_mprotect_pkey");
	do_mmap_addr = lookup_kallsyms_lookup_name("do_mmap");

	if (!bad_area_nosemaphore_addr || !do_mprotect_pkey_addr || !do_mmap_addr) {
		printk(KERN_ERR "Segmentation Guard 2: Failed to resolve needed symbols\n");
		return -ENOENT;
	}
	printk(KERN_INFO "Segmentation Guard 2: Found __bad_area_nosemaphore at %lx\nFound do_mprotect_pkey at %lx\nFound do_mmap at %lx\n", bad_area_nosemaphore_addr, do_mprotect_pkey_addr, do_mmap_addr);

	do_mprotect_pkey_fn = (do_mprotect_pkey_t) do_mprotect_pkey_addr;
	do_mmap_fn = (do_mmap_t) do_mmap_addr;

	if (register_kprobe(&bad_area_nosemaphore_kp) < 0) {
		printk(KERN_ERR "Segmentation Guard 2: Failed to register kprobe\n");
		return -EFAULT;
	}

	printk(KERN_INFO "Segmentation Guard 2: Kprobe registered successfully\n");
	return 0;
}

static void segmentation_guard_2_exit(void) {
	unregister_kprobe(&bad_area_nosemaphore_kp);
	printk(KERN_INFO "Segmentation Guard 2: Module unloaded successfully\n");
}

MODULE_DESCRIPTION("Segmentation Guard 2: A kernel module to fix segmentation faults");
MODULE_AUTHOR("blue5800");
MODULE_LICENSE("GPL");

module_init(segmentation_guard_2_init);
module_exit(segmentation_guard_2_exit);
