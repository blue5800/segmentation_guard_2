#pragma once

#include <linux/kprobes.h>
#include "sg2_interface.h"

extern enum sg2_status current_sg2_status;

int control_sg2(struct kprobe *kp, struct pt_regs *regs);
