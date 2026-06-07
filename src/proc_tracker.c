#include <linux/xarray.h>
#include "proc_tracker.h"

struct xarray sg2_enabled_pids;

void init_proc_tracker(void) {
	xa_init(&sg2_enabled_pids);
}

void exit_proc_tracker(void) {
	xa_destroy(&sg2_enabled_pids);
}

bool is_sg2_enabled_for_pid(pid_t pid) {
	return xa_load(&sg2_enabled_pids, pid) != NULL;
}

int enable_sg2_for_pid(pid_t pid) {
	if (is_sg2_enabled_for_pid(pid))
		return -EALREADY;

	void *tmp = xa_store(&sg2_enabled_pids, pid, xa_mk_value(1), GFP_ATOMIC);
	
	if (xa_is_err(tmp))
		return xa_err(tmp);
	
	printk(KERN_INFO "Segmentation Guard 2: Enabled SG2 for PID %d\n", pid);
	return 0;
}

void disable_sg2_for_pid(pid_t pid) {
	if (is_sg2_enabled_for_pid(pid)){
		xa_erase(&sg2_enabled_pids, pid);
		printk(KERN_INFO "Segmentation Guard 2: Disabled SG2 for PID %d\n", pid);
	}
}

int cleanup_proc_tracker_on_exit(struct kprobe *kp, struct pt_regs *regs){
	if (is_sg2_enabled_for_pid(current->tgid) && thread_group_empty(current)){
		disable_sg2_for_pid(current->tgid);
	}
	return 0;
}
