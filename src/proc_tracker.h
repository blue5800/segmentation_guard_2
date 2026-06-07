#pragma once 

#include <linux/kprobes.h>

void init_proc_tracker(void);
void exit_proc_tracker(void);

bool is_sg2_enabled_for_pid(pid_t pid);
int enable_sg2_for_pid(pid_t pid);
void disable_sg2_for_pid(pid_t pid);

int cleanup_proc_tracker_on_exit(struct kprobe *kp, struct pt_regs *regs);
