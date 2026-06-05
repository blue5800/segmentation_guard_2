#include <linux/kprobes.h>

#define SG2_MAGIC1 0xFEE1BAD
#define SG2_MAGIC2 0xDEADBEEF

enum sg2_control_cmds {
	SG2_CMD_STATUS = 1,
	SG2_CMD_GLOBAL_ENABLE = 2,
	SG2_CMD_GLOBAL_DISABLE = 3,
	SG2_CMD_ENABLE_THIS_PID = 4,
	SG2_CMD_DISABLE_THIS_PID = 5
};

enum sg2_status {
	SG2_STATUS_DISABLED = 0,
	SG2_STATUS_GLOBAL_ENABLED = 1,
	SG2_STATUS_PID_ENABLED = 2
};


typedef int (*do_mprotect_pkey_t)(unsigned long start, size_t len, unsigned long prot, int pkey);
typedef unsigned long (*do_mmap_t)(struct file *file, unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, vm_flags_t vm_flags, unsigned long pgoff, unsigned long *populate, struct list_head *uf);

extern enum sg2_status current_sg2_status;

extern do_mprotect_pkey_t do_mprotect_pkey_fn;
extern do_mmap_t do_mmap_fn;

extern unsigned long bad_area_nosemaphore_addr, do_mprotect_pkey_addr, do_mmap_addr;

int replace_bad_area_nosemaphore(struct kprobe *kp, struct pt_regs *regs);
int control_sg2(struct kprobe *kp, struct pt_regs *regs);
void boringpost(struct kprobe *kp, struct pt_regs *regs, unsigned long flags);
