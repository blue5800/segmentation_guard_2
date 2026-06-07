#include "sg2_control.h"
#include "bad_area_hook.h"

int control_sg2(struct kprobe *kp, struct pt_regs *regs){
	int magic1 = regs_get_kernel_argument(regs, 0);
	int magic2 = regs_get_kernel_argument(regs, 1);
	unsigned int cmd = regs_get_kernel_argument(regs, 2);
	int ret = 0;
	if(magic1 == SG2_MAGIC1 && magic2 == SG2_MAGIC2){
		switch (cmd){
			case SG2_CMD_STATUS:
				regs->ax = current_sg2_status;
				ret = 1;
				break;

// lets only let root enable/disable it globally
			case SG2_CMD_GLOBAL_ENABLE:
				if(!capable(CAP_SYS_ADMIN)){
					regs->ax = -EPERM;
					ret = 1;
					break;
				}
				current_sg2_status = SG2_STATUS_GLOBAL_ENABLED;
				regs->ax = 1;
				ret = 1;
				break;

			case SG2_CMD_GLOBAL_DISABLE:
				if(!capable(CAP_SYS_ADMIN)){
					regs->ax = -EPERM;
					ret = 1;
					break;
				}
				current_sg2_status = SG2_STATUS_DISABLED;
				regs->ax = 1;
				ret = 1;
				break;

// for these ones, since it only affects the current process ill just let them choose.
			case SG2_CMD_ENABLE_THIS_PID:
				regs->ax = -ENOSYS;
				ret = 1;
				break;

			case SG2_CMD_DISABLE_THIS_PID:
				regs->ax = -ENOSYS;
				ret = 1;
				break;
		}

	}
	if(ret)
		regs->ip = (unsigned long) boringpost;
	
	return ret;
}
