#pragma once

#include <linux/kprobes.h>

#ifdef CONFIG_FUNCTION_TRACER
#include <linux/ftrace.h>
#endif

typedef int (*do_mprotect_pkey_t)(unsigned long start, size_t len, unsigned long prot, int pkey);
typedef unsigned long (*do_mmap_t)(struct file *file, unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, vm_flags_t vm_flags, unsigned long pgoff, unsigned long *populate, struct list_head *uf);

extern do_mprotect_pkey_t do_mprotect_pkey_fn;
extern do_mmap_t do_mmap_fn;

extern unsigned long do_mprotect_pkey_addr, do_mmap_addr;

#ifdef CONFIG_FUNCTION_TRACER
void notrace sg2_ftrace_handler(unsigned long ip, unsigned long parent_ip, struct ftrace_ops *ops, struct ftrace_regs *regs);
#endif 

#ifdef CONFIG_X86_KERNEL_IBT
int do_mprotect_pkey_ibt(unsigned long start, size_t len, unsigned long prot, int pkey);
unsigned long do_mmap_ibt(struct file *file, unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, vm_flags_t vm_flags, unsigned long pgoff, unsigned long *populate, struct list_head *uf);
#endif

int replace_bad_area_nosemaphore(struct kprobe *kp, struct pt_regs *regs);
int control_sg2(struct kprobe *kp, struct pt_regs *regs);
void boringpost(struct kprobe *kp, struct pt_regs *regs, unsigned long flags);
