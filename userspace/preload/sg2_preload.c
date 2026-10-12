#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "sg2_interface.h"

__attribute__((constructor))
static void sg2_preload_init(void)
{
	long ret = syscall(SG2_SYSCALL_NUM, SG2_MAGIC1, SG2_MAGIC2, SG2_CMD_ENABLE_THIS_PID, NULL);
	if (ret == -1 && errno != EALREADY) {
		perror("[sg2_preload] Failed to enable SG2 for process");
	}
}
