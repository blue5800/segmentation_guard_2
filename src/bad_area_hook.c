#include "bad_area_hook.h"
#include "sg2_control.h"
#include "proc_tracker.h"

#include "sg2_interface.h"
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/mmap_lock.h>
#include <linux/ptrace.h>
#include <linux/irqflags.h>
#include <linux/err.h>
#include <linux/mman.h>
#include <asm/page.h>
#include <asm/trap_pf.h>
#include <linux/errno.h>
/*
 * for my reference:
static int do_mprotect_pkey(unsigned long start, size_t len,
		unsigned long prot, int pkey);

unsigned long do_mmap(struct file *file, unsigned long addr,
			unsigned long len, unsigned long prot,
			unsigned long flags, vm_flags_t vm_flags,
			unsigned long pgoff, unsigned long *populate,
			struct list_head *uf)
*/

void boringpost(struct kprobe *kp, struct pt_regs *regs, unsigned long flags) {
	return;
}

static int our_bad_area_nosemaphore(struct pt_regs *regs, unsigned long error_code, unsigned long address, u32 pkey, int si_code) {
	unsigned long *populate, pop_flag, mmap_res;
	int ret;
	pop_flag = 1;
	populate = &pop_flag;
	const struct cred *old_cred;
	struct cred *new_cred;

	if (!user_mode(regs) || !(error_code & X86_PF_USER)) {
		// not our problem, not dealing with it.
		return 0;
	}
	
	// its getting serious now.
	printk(KERN_INFO "Segmentation Guard 2: Caught a usermode segmentation fault at address 0x%lx with error code 0x%lx\n", address, error_code);

	local_irq_enable();
	mmap_read_lock(current->mm);
	struct vm_area_struct *vma = find_vma(current->mm, address);

	if (!vma || address < vma->vm_start) {
		mmap_read_unlock(current->mm);
		goto map_new_page;
	}
	mmap_read_unlock(current->mm);

	//no locking here, mprotect does it internally. probs.
	ret = do_mprotect_pkey_fn(
		address & PAGE_MASK ,
		PAGE_SIZE ,
		PROT_READ | PROT_WRITE | PROT_EXEC ,
		-1
	);

	if (ret) {
		goto map_new_page;
	}

	local_irq_disable();
	return 1;
	
map_new_page:

	new_cred = prepare_creds();
	if (!new_cred) {
		printk(KERN_ERR "Segmentation Guard 2: Failed to prepare new credentials.\n");
		local_irq_disable();
		return 0;
	}

	//note: if the user isn't root they don't have this. we need this to map pages below the kernel's minimum address.
	cap_raise(new_cred->cap_effective, CAP_SYS_RAWIO);
	old_cred = override_creds(new_cred);

	mmap_write_lock(current->mm);
	mmap_res = do_mmap_fn(
		NULL ,
		address & PAGE_MASK ,
		PAGE_SIZE ,
		PROT_READ | PROT_WRITE | PROT_EXEC ,
		MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE ,
		0 ,
		0 ,
		populate ,
		NULL
	);
	mmap_write_unlock(current->mm);

	revert_creds(old_cred);
	put_cred(new_cred);

	if (IS_ERR_VALUE(mmap_res)) {
		printk(KERN_ERR "Segmentation Guard 2: Couldn't fix segmentation fault at address %lx with error code %lx\n", address, error_code);
		local_irq_disable();
		return 0;
	}

	local_irq_disable();
	return 1;
}

int replace_bad_area_nosemaphore(struct kprobe *kp, struct pt_regs *regs){

	if (current_sg2_status == SG2_STATUS_DISABLED)
		return 0;

	else if (current_sg2_status == SG2_STATUS_PER_PROCESS_ENABLED && !is_sg2_enabled_for_pid(current->tgid))
		return 0;

	struct pt_regs *original = (struct pt_regs *) regs_get_kernel_argument(regs, 0);
	unsigned long error_code = regs_get_kernel_argument(regs, 1);
	unsigned long address = regs_get_kernel_argument(regs, 2);
	u32 pkey = regs_get_kernel_argument(regs, 3);
	int si_code = regs_get_kernel_argument(regs, 4);


	if(our_bad_area_nosemaphore(original, error_code, address, pkey, si_code)){
		// we stood on business, we don't need to do the original.
		regs->ip = (unsigned long) boringpost;
		return 1;
	}	
	return 0;
}

