#pragma once

#include <linux/kprobes.h>

typedef int (*do_mprotect_pkey_t)(unsigned long start, size_t len, unsigned long prot, int pkey);
typedef unsigned long (*do_mmap_t)(struct file *file, unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, vm_flags_t vm_flags, unsigned long pgoff, unsigned long *populate, struct list_head *uf);


extern do_mprotect_pkey_t do_mprotect_pkey_fn;
extern do_mmap_t do_mmap_fn;

extern unsigned long bad_area_nosemaphore_addr, do_mprotect_pkey_addr, do_mmap_addr;

int replace_bad_area_nosemaphore(struct kprobe *kp, struct pt_regs *regs);
int control_sg2(struct kprobe *kp, struct pt_regs *regs);
void boringpost(struct kprobe *kp, struct pt_regs *regs, unsigned long flags);
