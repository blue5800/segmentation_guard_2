# Segmentation Guard 2

**A kernel module that "fixes" segmentation faults by just... giving the process what it wants.**

>[!CAUTION]
>**Segmentation Guard 2 (SG2) is a catastrophic security hazard** Loading this module on a production system, or any system you care about, is a profoundly bad idea. This driver essentially dismantles 30 years worth of exploit mitigation, to keep your bad code running, while relying on runtime trickery to function. If you insist on loading this module regardless, I strongly advise you to change the default setting on load to `SG2_STATUS_DISABLED` or `SG2_STATUS_PER_PROCESS_ENABLED`.

### Why this is a terrible idea:

1.  **Destroys Security Boundaries:** This driver turns every memory corruption vulnerability (buffer overflows, use-after-frees, null pointer dereferences) into a feature. If a program tries to access memory it shouldn't, the kernel will now politely map that memory for it with **Read, Write, and Execute (RWX)** permissions.
2.  **Hides Critical Bugs:** Segmentation faults are a defense mechanism. They tell you when a program is broken. By "fixing" them, you allow broken programs to continue running in a corrupted state, leading to silent data corruption that is significantly harder to debug than a simple crash.
3.  **W^X Violation:** It maps memory as `PROT_READ | PROT_WRITE | PROT_EXEC`. This violates the fundamental security principle of "Write XOR Execute," making it trivial for an attacker to execute shellcode.
4.  **Resource Exhaustion:** A program with a memory leak or an infinite loop that touches new memory will no longer crash. Instead, it will keep asking the kernel for more pages until your system runs out of memory (OOM).
5.  **Kernel Instability:** The module hooks deep into the x86 page fault handling path (`__bad_area_nosemaphore`). While it now prefers the modern `ftrace` with `IPMODIFY` for stability, it still falls back to `kprobes` if ftrace is unavailable. It manually calls complex memory management functions (`do_mmap`, `do_mprotect_pkey`) from contexts where they may not be safe, potentially leading to kernel panics or deadlocks.

### Why i built it anyways:

1. [**Rule of cool:**](https://tvtropes.org/pmwiki/pmwiki.php/Main/RuleOfCool) This is objectively cool.
2. **Sequel to an existing project:** I previously wrote a userspace library `segmentation_guard` which skipped faulting instructions by decoding them in a signal handler and incrementing the IP past them. This was dependent on hacks like forcing the IP to point to a `ret` instruction when the code jumped to invalid memory, but there were ultimately many avenues to cause a SIGSEGV that couldn't be handled in that library. ring 0 grants us much greater flexibility in this regard, though.
3. **point number 3.**

---

## Technical Overview

SG2 intercepts calls to `__bad_area_nosemaphore`, the kernel function responsible for handling usermode page faults that occur outside of valid memory areas or violate permissions.

### Hooking Mechanism
The module employs a tiered hooking strategy to achieve function hijacking:
- **Primary (ftrace + IPMODIFY):** On kernels with `CONFIG_FUNCTION_TRACER` and `CONFIG_DYNAMIC_FTRACE_WITH_REGS`, SG2 uses the ftrace framework with the `IPMODIFY` flag. This allows for a clean redirect of the instruction pointer via `ftrace_regs_set_instruction_pointer` without the overhead or instability of breakpoint-based kprobes.
- **Fallback (kprobes):** If ftrace is unavailable, it falls back to a standard `kprobe` on the same symbol.

When a usermode fault is detected:
- **If the address is within an existing VMA:** It calls `do_mprotect_pkey` to upgrade the permissions of that page to `RWX`.
- **If the address is unmapped:** It calls `do_mmap` to create a new `MAP_FIXED` anonymous mapping at that address, also with `RWX` permissions.
- **If all else fails:** It resumes execution to the original `__bad_area_nosemaphore`, which sends a SIGSEGV to the program.

The original kernel fault handler is bypassed by redirecting the instruction pointer to a "landing pad" (`boringpost`), and the process resumes as if nothing went wrong.

### Intel CET / Indirect Branch Tracking (IBT) Bypass
On modern CPUs with Control-flow Enforcement Technology (CET) / Indirect Branch Tracking (IBT) enabled, invoking unexported kernel functions (like `do_mprotect_pkey` and `do_mmap`) indirectly via resolved function pointers normally triggers a CPU control-flow fault because their prologues lack an `ENDBR` instruction. 

SG2 bypasses this mitigation via a dynamic assembly stub patching mechanism:
1. **Assembly Stubs:** compliant assembly wrappers (`do_mprotect_pkey_ibt` and `do_mmap_ibt`) are defined in `src/ibt_wrappers.S`. These start with the required `ENDBR` instruction and are followed by a direct relative 32-bit jump (`jmp rel32`).
2. **Runtime Stub Patching:** In `src/segmentation_guard_2.c`, the `bad_idea` function temporarily maps the stub pages as writable via `vmap` to patch the jump target offsets to point to the resolved unexported kernel functions.
3. **Indirect Call Redirection:** The driver redirects its internal function pointer variables to the wrappers. Indirect calls land on the safe `ENDBR` instruction, then branch directly to the original kernel handlers via direct relative jumps (which are ignored by IBT).

SG2 also uses `kprobes` to intercept the `sys_reboot` syscall to use this as a control interface for the module. I chose to implement this control through this `sys_reboot` interface for no reason other than to save myself from the boilerplate of using ioctl or a character device. 

SG2 hooks `do_exit` to remove dead processes from its internal state, preventing reused PIDs from being automatically "guarded".

## Control Interface

SG2 can be controlled from userspace via a hijacked `reboot` system call. This allows you to check the status of the driver, enable/disable it globally, or enable/disable it for specific processes.

**Default State:**
By default, the driver initializes in **global protection mode** (`SG2_STATUS_GLOBAL_ENABLED`), automatically protecting all running user-space processes. This can be dynamically configured to per-process or disabled mode via the hijacked `reboot` control interface.

For detailed information on how to interact with the driver programmatically, see docs/INTERFACE.md

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
2. SG2 only works on specific architectures. I only support X86_64, it might (?) work on X86 but I have not tested it. Anything else is guaranteed to not work. 
3. You tell me.

## License
GPL
