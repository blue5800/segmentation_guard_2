# Segmentation Guard 2 Interface

The `segmentation_guard_2` (SG2) driver is controlled via a hijacked `reboot` system call (syscall 169). This allows user-space applications to query the status of the driver and enable or disable it globally or for specific processes.

## Control Mechanism

To interact with the driver, use the `reboot` syscall with the following parameters:

```c
long syscall(SYS_reboot, int magic1, int magic2, int cmd, void *arg);
```

### Magic Numbers

The driver expects two specific magic numbers in the first two arguments to identify the request as an SG2 control command:

*   `SG2_MAGIC1`: `0xFEE1BAD`
*   `SG2_MAGIC2`: `0xDEADBEEF`

### Commands (`cmd`)

The following commands are supported:

| Command | Value | Description | Required Capability |
| :--- | :--- | :--- | :--- |
| `SG2_CMD_STATUS` | 1 | Returns the current global status of SG2. | None |
| `SG2_CMD_GLOBAL_ENABLE` | 2 | Enables SG2 for all processes. | `CAP_SYS_ADMIN` |
| `SG2_CMD_GLOBAL_DISABLE` | 3 | Disables SG2 globally. | `CAP_SYS_ADMIN` |
| `SG2_CMD_PER_PROCESS_ENABLE` | 4 | Sets SG2 to per-process mode. | `CAP_SYS_ADMIN` |
| `SG2_CMD_ENABLE_THIS_PID` | 5 | Enables SG2 for the calling process. | None |
| `SG2_CMD_DISABLE_THIS_PID` | 6 | Disables SG2 for the calling process. | None |

**Note:** "process" refers to a thread group, rather than an individual thread.

### Return Values

*   For `SG2_CMD_STATUS`, it returns one of the following status values:
    *   `SG2_STATUS_DISABLED` (0)
    *   `SG2_STATUS_GLOBAL_ENABLED` (1)
    *   `SG2_STATUS_PER_PROCESS_ENABLED` (2)
*   For other commands:
    *   `1` on success (for global commands).
    *   `0` on success (for `SG2_CMD_ENABLE_THIS_PID` and `SG2_CMD_DISABLE_THIS_PID`).
    *   `-EPERM` (-1) if the caller lacks the required capabilities.
    *   `-EALREADY` (-114) if SG2 is already enabled for the PID.

## Status Enums

```c
enum sg2_status {
	SG2_STATUS_DISABLED = 0,
	SG2_STATUS_GLOBAL_ENABLED = 1,
	SG2_STATUS_PER_PROCESS_ENABLED = 2
};
```

## Example Usage

```c
#include <unistd.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <stdio.h>
#include "sg2_interface.h"

int main() {
    // Check current status
    int status = syscall(SYS_reboot, SG2_MAGIC1, SG2_MAGIC2, SG2_CMD_STATUS, NULL);
    printf("Current SG2 Status: %d\n", status);

    // Enable SG2 for this process
    int ret = syscall(SYS_reboot, SG2_MAGIC1, SG2_MAGIC2, SG2_CMD_ENABLE_THIS_PID, NULL);
    if (ret == 0) {
        printf("SG2 enabled for this process.\n");
    } else {
        perror("Failed to enable SG2");
    }

    return 0;
}
```

The header file `sg2_interface.h` should be included in your project to use these constants and enums.
