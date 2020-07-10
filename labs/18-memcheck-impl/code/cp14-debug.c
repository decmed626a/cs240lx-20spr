// handle debug exceptions.
#include "rpi.h"
#include "rpi-interrupts.h"
#include "libc/helper-macros.h"
#include "cp14-debug.h"
#include "bit-support.h"

/******************************************************************
 * part1 set a watchpoint on 0.
 )*/
static handler_t watchpt_handler0 = 0;

cp_asm_set(prefetch_flush, p15, 0, c7, c5, 4)

// prefetch flush.
#define prefetch_flush() prefetch_flush_set(0)

// cp14_asm_get(wfar, c6, 0)

//static inline uint32_t cp14_wcr0_get(void) {
//    unimplemented();
//}

cp14_asm_get(wcr0, c0, 7)

//static inline void cp14_wcr0_set(uint32_t r) {
//    unimplemented();
//}

cp14_asm_set(wcr0, c0, 7) 

//static inline uint32_t cp14_wvr0_get(void) { unimplemented(); }

cp14_asm_get(wvr0, c0, 6)

//static inline void cp14_wvr0_set(uint32_t r) { unimplemented(); }

cp14_asm_set(wvr0, c0, 6)

//static inline uint32_t cp14_status_get(void) {
//    unimplemented();
//}

cp14_asm_get(dscr, c1, 0) 

//static inline void cp14_status_set(uint32_t v) {
//    unimplemented();
//}

cp14_asm_set(dscr, c1, 0)


// set the first watchpoint: calls <handler> on debug fault.
void watchpt_set0(void *_addr, handler_t handler) {
	
	// Read WCR0
	uint32_t wcr0_reg = wcr0_get();
	
	// Disable watchpoint	
	wcr0_reg &= ~(1 << 0);

	// Write back to WCR0
	wcr0_set(wcr0_reg);

	assert(wcr0_reg == wcr0_get());
	
	uint32_t wvr0_val = (uint32_t)_addr;

	// Write address to WVR 
	wvr0_set(wvr0_val);

	wcr0_reg = 0;
	wcr0_reg = (0b0 << 20) |
			   (0b00 << 14) |
			   (0b1111 << 5) |
			   (0b11 << 3) |
			   (0b11 << 1) | 
			   (0b1 << 0);

	wcr0_set(wcr0_reg);
   
	assert(wcr0_reg == wcr0_get());

	prefetch_flush();
    watchpt_handler0 = handler;
}

#if 0
// check for watchpoint fault and call <handler> if so.
void data_abort_vector(unsigned pc) {
	static int nfaults = 0;
    //printk("nfault=%d: data abort at %p\n", nfaults++, pc);
    if(datafault_from_ld()) {
       // printk("was from a load\n");
	 }
    else {
        //printk("was from a store\n");
	}
	if(!was_debug_datafault()) 
        panic("impossible: should get no other faults\n");

    // this is the pc
    //printk("wfar address = %p, pc = %p\n", wfar_get()-8, pc);
    assert(wfar_get()-4 == pc);

    assert(watchpt_handler0);
    
    uint32_t addr = far_get();
    //printk("far address = %p\n", addr);

    // should send all the registers so the client can modify.
    watchpt_handler0(0, pc, addr);
#endif 
	// b4-20
enum {
    SECTION_XLATE_FAULT = 0b00101,
    SECTION_PERM_FAULT = 0b01001,
};

// remove all non-section bits.


typedef struct {
	unsigned status : 4;
	unsigned domain : 4;
	unsigned zero : 1;
	unsigned sbz_1 : 1;
	unsigned fs_4 : 1;
	unsigned wr : 1;
	unsigned sbz_2 : 20;
} dfsr_t;


typedef enum {
	ALIGNMENT = 0b00001,
	TLB_MISS = 0b0,
	ALIGNMENT_DEP = 0b00011,
	TRANSLATION_SECTION = 0b00101,
	PERMISSION_SECTION = 0b01101,
} fault_status_t;

void data_abort_vector(unsigned lr) {
    //x_mmu_disable();
    //panic("in data abort\n");

	dfsr_t dfsr = {0};
	//uint32_t fsr = 0;
	uint32_t far = 0;
	asm volatile ("MRC p15, 0, %0, c5, c0, 0" : "=r"(dfsr));
	asm volatile ("MRC p15, 0, %0, c6, c0, 0" : "=r"(far));

	//printk("FSR is: %x\n", fsr);

	printk("FAR is: %x\n", far);

	unsigned reason = dfsr.fs_4 << 4 | dfsr.status;
	unsigned mask_offset = ~0xFFFFF;
	switch(reason) {
		case ALIGNMENT:
			x_mmu_disable();
			panic("Alignment fault\n");
			break;
		case TLB_MISS:
			x_mmu_disable();
			panic("TLB Miss \n");
			break;
		case ALIGNMENT_DEP:
			x_mmu_disable();
			panic("Dep Alignment fault\n");
			break;
		case TRANSLATION_SECTION:
			//panic("In translation");
			printk("Translation fault\n");
			mmu_map_section(pt_st, far & mask_offset, far & mask_offset);
			break;
		case PERMISSION_SECTION:
			//panic("In permission");
			printk("Permission fault\n");
			mmu_mark_sec_rw (pt_st, far & mask_offset, 1);
			break;
		default:
			x_mmu_disable();
			panic("WTF case is this!?!??");
	}
	mmu_sync_pte_mod();
}

void cp14_enable(void) {
    static int init_p = 0;

    if(!init_p) { 
        int_init();
        init_p = 1;
    }

    // for the core to take a debug exception, monitor debug mode has to be both 
    // selected and enabled --- bit 14 clear and bit 15 set.
	uint32_t v = 0;
	v = (1 << 15);
	dscr_set(v);

    prefetch_flush();
    // assert(!cp14_watchpoint_occured());

}

/**************************************************************
 * part 2
 */

static handler_t brkpt_handler0 = 0;

cp14_asm_get(cp14_bvr0, c0, 4)
cp14_asm_set(cp14_bvr0, c0, 4)
cp14_asm_get(cp14_bcr0, c0, 5)
cp14_asm_set(cp14_bcr0, c0, 5)
//static inline uint32_t cp14_bvr0_get(void) { unimplemented(); }
//static inline void cp14_bvr0_set(uint32_t r) { unimplemented(); }
//static inline uint32_t cp14_bcr0_get(void) { unimplemented(); }
//static inline void cp14_bcr0_set(uint32_t r) { unimplemented(); }

static unsigned bvr_match(void) { return 0b00 << 21; }
static unsigned bvr_mismatch(void) { return 0b10 << 21; }

static inline uint32_t brkpt_get_va0(void) {
    return cp14_bvr0_get();
}
static void brkpt_disable0(void) {
	uint32_t bcr0_reg = 0;
	bcr0_reg = (0b00 << 21) |
			   (0b0 << 20) |
			   (0b00 << 14) |
			   (0b1111 << 5) |
			   (0b11 << 1) | 
			   (0b0 << 0);

	cp14_bcr0_set(bcr0_reg);
}

// 13-16
// returns the 
void brkpt_set0(uint32_t addr, handler_t handler) {
	uint32_t bcr0_reg = cp14_bcr0_get();

	// Disable watchpoint	
	bcr0_reg &= ~(1 << 0);

	// Write back to BCR0
	cp14_bcr0_set(bcr0_reg);

	uint32_t bvr0_val = (uint32_t)addr;

	// Write address to WVR 
	cp14_bvr0_set(bvr0_val);

	bcr0_reg = 0;
	bcr0_reg = (0b00 << 21) |
			   (0b0 << 20) |
			   (0b00 << 14) |
			   (0b1111 << 5) |
			   (0b11 << 1) | 
			   (0b1 << 0);

	cp14_bcr0_set(bcr0_reg);
	prefetch_flush();
	brkpt_handler0 = handler;
}

// if get a breakpoint call <brkpt_handler0>
void prefetch_abort_vector(unsigned pc) {
    //printk("prefetch abort at %p\n", pc);
    if(!was_debug_instfault())
        panic("impossible: should get no other faults\n");
    assert(brkpt_handler0);
    //printk("After assert\n", pc);
    brkpt_handler0(0, pc, ifar_get());
}

void interrupt_vector(unsigned pc) {
    
}

/**************************************************************
 * part 2
 */

int brk_verbose_p = 0;

// used by trampoline to catch if the code returns.
void brk_no_ret_error(void) {
    panic("returned and should not have!\n");
}

void brkpt_mismatch_set0(uint32_t addr, handler_t handler) {
	uint32_t bcr0_reg = cp14_bcr0_get();

	// Disable watchpoint	
	bcr0_reg &= ~(1 << 0);

	// Write back to BCR0
	cp14_bcr0_set(bcr0_reg);

	uint32_t bvr0_val = (uint32_t)addr;

	// Write address to WVR 
	cp14_bvr0_set(bvr0_val);

	bcr0_reg = 0;
	bcr0_reg = (0b10 << 21) |  // Set to 0b10 for address mismatch
			   (0b0 << 20) |
			   (0b00 << 14) |
			   (0b1111 << 5) |
			   (0b11 << 1) | 
			   (0b1 << 0);

	cp14_bcr0_set(bcr0_reg);
	prefetch_flush();
	brkpt_handler0 = handler;
}

// should be this addr.
void brkpt_mismatch_disable0(uint32_t addr) {
	uint32_t bcr0_reg = 0;
	bcr0_reg = (0b10 << 21) |  // Set to 0b10 for address mismatch
			   (0b0 << 20) |
			   (0b00 << 14) |
			   (0b1111 << 5) |
			   (0b11 << 1) | 
			   (0b0 << 0);
	cp14_bcr0_set(bcr0_reg);
}

// <pc> should point to the system call instruction.
//      can see the encoding on a3-29:  lower 24 bits hold the encoding.
// <r0> = the first argument passed to the system call.
//
// system call numbers:
//  <1> - set spsr to the value of <r0>
int syscall_vector(unsigned pc, uint32_t r0) {
    uint32_t sys_num;
    // figure out the instruction and the system call number.
    sys_num = *((unsigned * ) pc) & 0xFFFFFF;
    printk("sys_num = %d\n", sys_num);
	if(sys_num == 1){
		asm volatile ("msr spsr, r1");
    }else if(sys_num == 2){
		// if can take lock, return 1
		if(*(int*)r0 == 0) {
			*(int*)r0 = 1;
			return 1;
		} else {
			return 0;
		}
	}else if (sys_num == 3) {
		return 3;
		//printk("result: %x\n", r0);
    }else if (sys_num == 10) {
		printk("In sys 10\n");
		return 10;
	}else{
        printk("illegal system call %d\n", sys_num);
    }

    return 0;
}
