	.text
	.balign 4096

	.globl do_mprotect_pkey_ibt
	.type do_mprotect_pkey_ibt, @function
do_mprotect_pkey_ibt:
#ifdef CONFIG_X86_64
	endbr64
#else
	endbr32
#endif
	.byte 0xe9
	.long 0x0
	ret //unreachable
	.size do_mprotect_pkey_ibt, .-do_mprotect_pkey_ibt

	.balign 16
	.globl do_mmap_ibt
	.type do_mmap_ibt, @function
do_mmap_ibt:
#ifdef CONFIG_X86_64
	endbr64
#else
	endbr32
#endif
	.byte 0xe9
	.long 0x0
	ret
	.size do_mmap_ibt, .-do_mmap_ibt

	.balign 4096
	# Dummy function to satisfy objtool
	.globl ibt_wrappers_barrier_func
	.type ibt_wrappers_barrier_func, @function
ibt_wrappers_barrier_func:
#ifdef CONFIG_X86_64
	endbr64
#else
	endbr32
#endif
	ret
	.size ibt_wrappers_barrier_func, .-ibt_wrappers_barrier_func
