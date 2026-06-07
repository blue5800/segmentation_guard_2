# Segmentation Guard 2

**A kernel module that "fixes" segmentation faults by just... giving the process what it wants.**

## ⚠️ EXTREME WARNING: DO NOT USE THIS ⚠️

**Segmentation Guard 2 (SG2) is a catastrophic security and stability hazard.** Loading this module on a production system—or any system you care about—is effectively an act of digital self-sabotage. 

### Why this is a terrible idea:

1.  **Destroys Security Boundaries:** This driver turns every memory corruption vulnerability (buffer overflows, use-after-frees, null pointer dereferences) into a feature. If a program tries to access memory it shouldn't, the kernel will now politely map that memory for it with **Read, Write, and Execute (RWX)** permissions.
2.  **Hides Critical Bugs:** Segmentation faults are a defense mechanism. They tell you when a program is broken. By "fixing" them, you allow broken programs to continue running in a corrupted state, leading to silent data corruption that is significantly harder to debug than a simple crash.
3.  **W^X Violation:** It maps memory as `PROT_READ | PROT_WRITE | PROT_EXEC`. This violates the fundamental security principle of "Write XOR Execute," making it trivial for an attacker to execute shellcode.
4.  **Resource Exhaustion:** A program with a memory leak or an infinite loop that touches new memory will no longer crash. Instead, it will keep asking the kernel for more pages until your system runs out of memory (OOM).
5.  **Kernel Instability:** The module hooks deep into the x86 page fault handling path (`__bad_area_nosemaphore`) using kprobes. It manually toggles interrupts and calls complex memory management functions (`do_mmap`, `do_mprotect_pkey`) from contexts where they may not be safe, potentially leading to kernel panics or deadlocks.

### Why i built it anyways:

1. [**Rule of cool:**](https://tvtropes.org/pmwiki/pmwiki.php/Main/RuleOfCool) This is objectively cool.
2. **Sequel to an existing project:** I previously wrote a userspace library `segmentation_guard` which skipped faulting instructions by decoding them in a signal handler and incrementing the IP past them. This was dependent on hacks like forcing the IP to point to a `ret` instruction when the code jumped to invalid memory, but there were ultimately many avenues to cause a SIGSEGV that couldn't be handled in that library. ring 0 grants us much greater flexibility in this regard, though.
3. **point number 3.**

---

## Technical Overview

SG2 uses `kprobes` to intercept calls to `__bad_area_nosemaphore`, the kernel function responsible for handling usermode page faults that occur outside of valid memory areas or violate permissions.

When a usermode fault is detected:
- **If the address is within an existing VMA:** It calls `do_mprotect_pkey` to upgrade the permissions of that page to `RWX`.
- **If the address is unmapped:** It calls `do_mmap` to create a new `MAP_FIXED` anonymous mapping at that address, also with `RWX` permissions.
- **If all else fails:** It resumes execution to the original __bad_area_nosemaphore, which sends a SIGSEGV to the program.

The original kernel fault handler is then bypassed, and the process resumes as if nothing went wrong.

SG2 also uses `kprobes` to intercept the `sys_reboot` syscall to use this as a control interface for the module. I chose to implement this control through this `sys_reboot` interface for no reason other than to save mmyself from the boilerplate of using ioctl or a character device. 

SG2 hooks `do_exit` to remove dead processes from its internal state, preventing reused PIDs from being automatically "guarded"

## Control Interface

SG2 can be controlled from userspace via a hijacked `reboot` system call. This allows you to check the status of the driver, enable/disable it globally, or enable/disable it for specific processes.

For detailed information on how to interact with the driver programmatically, see [docs/INTERFACE.md](docs/INTERFACE.md).

## Build & Installation

*Requires kernel headers, a kernel with CONFIG_KPROBES, and a compiler.*

```bash
make
sudo insmod segmentation_guard_2.ko
```

To return to sanity:
```bash
sudo rmmod segmentation_guard_2
```

Alternatively: it can be built in-tree by sourcing the Kconfig, and enabling the module in your defconfig. 

## Examples:
See userspace/examples/ for examples of userspace programs which this module "fixes"
Small standalone program(s) which require no special linkage therefore I omitted a Makefile for these.

## sg2ctl:
Small C program which can be used to control and read the active SG2 mode.

## Limitations:
1. The kernel module has only been tested on linux-7.1-rc6 and 7.0.11-arch1-1. if the internal ABI in your kernel is different, it won't work and it will likely crash spectacularly.
2. SG2 only works on specific architectures. I only support X86_64, it will likely work on X86 but I have not tested it. Anything else is guaranteed to not work. 
3. You tell me.

## License
GPL
