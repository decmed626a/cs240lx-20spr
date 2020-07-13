// engler: make sure we can call a system call at user level.
#include "memcheck.h"

int notmain_client() {
    int sys_10_asm(void);

    int ret = sys_10_asm();
	printk("syscal 10 =%d\n", ret);
    assert(ret == 10);
	printk("returning in notmain_client\n");
    return 0x12345678;
}

void notmain() {
	assert(!mmu_is_enabled());

    assert(mode_is_super());
    int x = memcheck_fn(notmain_client);
    printk("back in notmain\n");
	assert(x == 0x12345678);
    assert(mode_is_super());
    

    assert(!mmu_is_enabled());

    trace_clean_exit("success!!\n");
}
