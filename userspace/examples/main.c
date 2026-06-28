#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "sg2_interface.h"

static const char *sg2_status_name(long status)
{
	switch (status) {
	case SG2_STATUS_DISABLED:
		return "disabled";
	case SG2_STATUS_GLOBAL_ENABLED:
		return "globally enabled";
	case SG2_STATUS_PER_PROCESS_ENABLED:
		return "per-process enabled";
	default:
		return "unknown";
	}
}

static long sg2_command(enum sg2_control_cmds cmd)
{
	return syscall(SG2_SYSCALL_NUM, SG2_MAGIC1, SG2_MAGIC2, cmd, NULL);
}

static int ensure_sg2_protected(void)
{
	long status = sg2_command(SG2_CMD_STATUS);

	if (status == -1) {
		fprintf(stderr, "Failed to check SG2 status: %s\n", strerror(errno));
		return -1;
	}

	printf("SG2 status: %s (%ld)\n", sg2_status_name(status), status);

	switch (status) {
	case SG2_STATUS_GLOBAL_ENABLED:
		return 0;
	case SG2_STATUS_PER_PROCESS_ENABLED: {
		long ret = sg2_command(SG2_CMD_ENABLE_THIS_PID);

		if (ret == -1 && errno != EALREADY) {
			fprintf(stderr, "Failed to enable SG2 for this process: %s\n",
				strerror(errno));
			return -1;
		}

		printf("SG2 protection enabled for this process.\n");
		return 0;
	}
	case SG2_STATUS_DISABLED:
		fprintf(stderr, "SG2 is disabled; refusing to run unsafe memory accesses.\n");
		return -1;
	default:
		fprintf(stderr, "Unknown SG2 status %ld; refusing to continue.\n", status);
		return -1;
	}
}

int main(void)
{
	if (ensure_sg2_protected() != 0)
		return 1;

	char *msg = (char *)main;

	int *x = (int *)0;
	printf("%d\n", *x);

	*x = 4096;
	printf("%d\n", *x);

	msg[0] = 'H';
	msg[1] = 'i';
	msg[2] = '\0';
	
	printf("%s\n", msg);
	printf("i'm so smart :3\n");

	return 0;
}
