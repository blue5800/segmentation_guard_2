#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include "sg2_interface.h"

void print_usage(const char *prog_name) {
    printf("Usage: %s <command>\n", prog_name);
    printf("Commands:\n");
    printf("  status       - Get current SG2 status\n");
    printf("  enable       - Enable SG2 globally\n");
    printf("  disable      - Disable SG2 globally\n");
    printf("  per-process  - Set SG2 to per-process mode\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd_str = argv[1];
    int cmd = 0;

    if (strcmp(cmd_str, "status") == 0) {
        cmd = SG2_CMD_STATUS;
    } else if (strcmp(cmd_str, "enable") == 0) {
        cmd = SG2_CMD_GLOBAL_ENABLE;
    } else if (strcmp(cmd_str, "disable") == 0) {
        cmd = SG2_CMD_GLOBAL_DISABLE;
    } else if (strcmp(cmd_str, "per-process") == 0) {
        cmd = SG2_CMD_PER_PROCESS_ENABLE;
    } else {
        printf("Unknown command: %s\n", cmd_str);
        print_usage(argv[0]);
        return 1;
    }

    long ret = syscall(SG2_SYSCALL_NUM, SG2_MAGIC1, SG2_MAGIC2, cmd, NULL);

    if (cmd == SG2_CMD_STATUS) {
        if (ret == SG2_STATUS_DISABLED) {
            printf("SG2 Status: Disabled (0)\n");
        } else if (ret == SG2_STATUS_GLOBAL_ENABLED) {
            printf("SG2 Status: Globally Enabled (1)\n");
        } else if (ret == SG2_STATUS_PER_PROCESS_ENABLED) {
            printf("SG2 Status: Per-Process Enabled (2)\n");
        } else if (ret == -1) {
            perror("Failed to get SG2 status");
            return 1;
        } else {
            printf("SG2 Status: Unknown result from driver (%ld)\n", ret);
        }
    } else {
        if (ret == -1) {
            perror("Command failed");
            return 1;
        } else {
            printf("Command '%s' executed successfully (ret=%ld).\n", cmd_str, ret);
        }
    }

    return 0;
}
