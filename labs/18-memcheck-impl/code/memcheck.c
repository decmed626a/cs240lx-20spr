/*
 * engler: cs340lx
 *
 * start of a memchecker.  sort of lame in that you have to deal with a bunch of 
 * my starter code (sorry) --- the issue is that it might not be clear how to 
 * call the vm.
 *
 * implement:
 *  - static void dom_perm_set(unsigned dom, unsigned perm) {
 *  - void data_abort_vector(unsigned lr) {
 * (1) is trivial, but just to remind you about domain id's.
 * (2) involves computing the fault reason and then acting.
 *
 */
#include "memcheck.h"
#include "cp14-debug.h"
#include "libc/helper-macros.h"

int run_fn_helper(uint32_t cpsr, void (*fn)(void), uint32_t sp);

enum { OneMB = 1024 * 1024 };

// don't use dom id = 0 --- too easy to miss errors.
enum { dom_id = 1, track_id = 2 };

static fld_t* pt_st = 0;

/**********************************************************************
 * helper code to track the last fault (used for testing).
 */
static last_fault_t last_fault;
last_fault_t last_fault_get(void) {
    return last_fault;
}
void last_fault_set(uint32_t fault_pc, uint32_t fault_addr, uint32_t reason) {
    last_fault = (last_fault_t) { 
            .fault_addr = fault_addr, 
            .fault_pc = fault_pc,
            .fault_reason = reason
    };
}
void fault_expected(uint32_t pc, uint32_t addr, uint32_t reason) {
    last_fault_t l = last_fault_get();
    assert(l.fault_addr == addr);
    assert(l.fault_pc ==  pc);
    assert(l.fault_reason == reason);
    trace("saw expected fault at pc=%p, addr=%p, reason=%d\n", pc,addr,reason);
}

/****************************************************************
 * mechanical code for flipping permissions for the tracked memory
 * domain <track_id> on or off.
 *
   from in the armv6.h header:
    DOM_no_access = 0b00, // any access = fault.
    DOM_client = 0b01,      // client accesses check against permission bits in tlb
    DOM_reserved = 0b10,      // client accesses check against permission bits in tlb
    DOM_manager = 0b11,      // TLB access bits are ignored.
 */

static unsigned dom_perm_get(unsigned dom) {
    unsigned x = read_domain_access_ctrl();
    return bits_get(x, dom*2, (dom+1)*2);
}

// set the permission bits for domain id <dom> to <perm>
// leave the other domains the same.
static void dom_perm_set(unsigned dom, unsigned perm) {
    assert(dom < 16);
    assert(perm == DOM_client || perm == DOM_no_access);

	unsigned dac = (read_domain_access_ctrl() &
					(~(0b11 << dom * 2))) | (perm << dom * 2);
	write_domain_access_ctrl(dac);
}


/**************************************************************************
 * handle a trap.
 */

// hack to turn off resume / not resume for testing.
static unsigned mmu_resume_p = 0;
void memcheck_continue_after_fault(void) {
    mmu_resume_p = 1;
}

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
	PMSA_TLB_MISS = 0b00000,
	ALIGNMENT_DEPR = 0b00011,
	ICACHE_MAINT_OP_FT = 0b00100,
	EXT_ABT_TRANSLATION_LVL_1 = 0b01100,
	EXT_ABT_TRANSLATION_LVL_2 = 0b01110,
	TRANSLATION_SECTION = 0b00101,
	TRANSLATION_PAGE = 0b00111,
	DOMAIN_SECTION = 0b01001,
	DOMAIN_PAGE = 0b01011,
	PERMISSION_SECTION = 0b01101,
	PERMISSION_PAGE = 0b01111,
	PRECISE_EXT_ABT = 0b01000,
	EXT_ABT_PREC_DEPR = 0b01010,
	TLB_LOCK = 0b10100,
	COPROC_DATA_ABT = 0b11010,
	IMPREC_EXT_ABT = 0b10110,
	PARITY_ERR_EXCP = 0b11000,
	DEBUG_EVENT = 0b00010,
} dfsr_reason_t;

// simple data abort handle: handle the different reasons for the tests.
void data_abort_vector(unsigned lr) {
#if 0
	if(was_debug_datafault())
        mem_panic("should not have this\n");
#endif 
    // b4-43
    unsigned dfsr = dfsr_get();
    unsigned fault_addr = far_get();

    // compute the rason.
    //unsigned reason = 0;

	printk("In data abort vector\n");

	//unsigned reason = dfsr.fs_4 << 4 | dfsr.status;
	unsigned reason = (dfsr & 0b1111) | (dfsr & (0b1 << 10)) >> 6;
    last_fault_set(lr, fault_addr, reason);

	switch(reason) {
		case ALIGNMENT:
			panic("Alignment fault\n");
			break;
		case PMSA_TLB_MISS:
			panic("TLB Miss \n");
			break;
		case ALIGNMENT_DEPR:
			panic("Dep Alignment fault\n");
			break;
		case ICACHE_MAINT_OP_FT:
			panic("ICache Maintenance fault\n");
			break;
		case EXT_ABT_TRANSLATION_LVL_1:
			panic("Translation Level 1 fault\n");
			break;
		case EXT_ABT_TRANSLATION_LVL_2:
			panic("Translation level 2 fault\n");
			break;
		case TRANSLATION_SECTION:
            trace_clean_exit("section xlate fault: addr=%p, at pc=%p\n"); 
			break;
		case TRANSLATION_PAGE:
			panic("Translation page fault\n");
			break;
		case DOMAIN_SECTION:
			printk("Domain section fault\n");
            if(!mmu_resume_p)
                trace_clean_exit("Going to die!\n");
            else {
                trace("going to try to resume\n");
                memcheck_trap_disable();
                return;
            }
			break;
		case DOMAIN_PAGE:
			panic("Domain page fault\n");
			break;
		case PERMISSION_SECTION:
            panic("section permission fault: %x", fault_addr);
			break;
		case PERMISSION_PAGE:
			panic("Permission page fault\n");
			break;
		case PRECISE_EXT_ABT:
			panic("Precise external abort\n");
			break;
		case EXT_ABT_PREC_DEPR:
			panic("External abort precise\n");
			break;
		case TLB_LOCK:
			panic("TLB Lock\n");
			break;
		case COPROC_DATA_ABT:
			panic("Coprocessor data abort\n");
			break;
		case IMPREC_EXT_ABT:
			panic("Imprecise external abort\n");
			break;
		case PARITY_ERR_EXCP:
			panic("Parity error exception\n");
			break;
		case DEBUG_EVENT:
			panic("Debug event\n");
			break;
		default:
			panic("WTF case is this!?!??");
	}

    /*
         for SECTION_XLATE_FAULT:
            trace_clean_exit("section xlate fault: addr=%p, at pc=%p\n" 
         for DOMAIN_SECTION_FAULT:
            if(!mmu_resume_p)
                trace_clean_exit("Going to die!\n");
            else {
                trace("going to try to resume\n");
                memcheck_trap_disable();
                return;
            }
        for SECTION_PERM_FAULT:
            panic("section permission fault: %x", fault_addr);

        otherwise
        default: panic("unknown reason %b\n", reason);
     */ 
}

// shouldn't happen: need to fix libpi so we don't have to do this.
void interrupt_vector(unsigned lr) {
    mmu_disable();
    panic("impossible\n");
}


/*************************************************************************
 * helper code: you don't have to write this; it's based on cs140e.
 */

// XXX: our page table, gross.
static fld_t *pt = 0;

// need some parameters for this.
void memcheck_init(void) {
    // 1. init
    mmu_init();
	
	assert(!mmu_is_enabled());

    void *base = (void*)0x100000;

    pt = mmu_pt_alloc_at(base, 4096*4);
    assert(pt == base);

    // 2. setup mappings

    // map the first MB: shouldn't need more memory than this.
    mmu_map_section(pt, 0x0, 0x0, dom_id);
    // map the page table: for lab cksums must be at 0x100000.
    mmu_map_section(pt, 0x100000,  0x100000, dom_id);
    // map stack (grows down)
    mmu_map_section(pt, STACK_ADDR-OneMB, STACK_ADDR-OneMB, dom_id);
    mmu_map_section(pt, STACK_ADDR2-OneMB, STACK_ADDR2-OneMB, dom_id);

    // map the GPIO: make sure these are not cached and not writeback.
    // [how to check this in general?]
    mmu_map_section(pt, 0x20000000, 0x20000000, dom_id);
    mmu_map_section(pt, 0x20100000, 0x20100000, dom_id);
    mmu_map_section(pt, 0x20200000, 0x20200000, dom_id);

    // if we don't setup the interrupt stack = super bad infinite loop
    mmu_map_section(pt, INT_STACK_ADDR-OneMB, INT_STACK_ADDR-OneMB, dom_id);

    // 3. install fault handler to catch if we make mistake.
    mmu_install_handlers();

    // 4. start the context switch:

    // set up permissions for the different domains: we only use <dom_id>
    // and permissions r/w.
    write_domain_access_ctrl(0b01 << dom_id*2);

    // use the sequence on B2-25
    set_procid_ttbr0(0x140e, dom_id, pt);
}

// turn memchecking on: right now, just enable the mmu (which should 
// flush any needed caches).
void memcheck_on(void) {
    debug("memcheck: about to turn ON\n");
    // note: this routine has to flush I/D cache and TLB, BTB, prefetch buffer.
    mmu_enable();
    assert(mmu_is_enabled());
    debug("memcheck: ON\n");
}
// turn memcheck off: right now just diable mmu (which should flush
// any needed caches)
void memcheck_off(void) {
    // 6. disable.
    mmu_disable();
    assert(!mmu_is_enabled());
    debug("memcheck: OFF\n");
}

void memcheck_map(uint32_t base) {
    assert(is_aligned(base,OneMB));

    // XXX: need to handle when it's already mapped.
    mmu_map_section(pt, base, base + OneMB, dom_id);
}

void memcheck_track(uint32_t base) {
    assert(is_aligned(base,OneMB));

    // XXX: need to handle when it's already mapped.
    mmu_map_section(pt, base, base + OneMB, track_id);
}

void memcheck_no_access(uint32_t base) {
    base &= ~(OneMB - 1);
    assert(is_aligned(base,OneMB));
    mmu_mark_sec_no_access(pt, base, 1);
}

// is trapping enabled?
unsigned memcheck_trap_enabled(void) {
    return dom_perm_get(track_id) == DOM_no_access;
}

// disable traps on tracked memory: we still do permission based checks
// (you can disable this using DOM_manager).
void memcheck_trap_disable(void) {
    dom_perm_set(track_id, DOM_client);
    assert(!memcheck_trap_enabled());
}
void memcheck_trap_enable(void) {
    dom_perm_set(track_id, DOM_no_access);
    assert(memcheck_trap_enabled());
}

int memcheck_fn(int (*fn)(void)) {
	int_init();
	int result = run_fn_helper(USER_MODE,(void (*) (void)) &fn, STACK_ADDR2);
	return result;
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
    //printk("In syscall_vector\n");
	sys_num = *((unsigned * ) pc) & 0xFFFFFF;
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
		printk("In syscall 3\n");
		return 3;
		//printk("result: %x\n", r0);
    }else if (sys_num == 10) {
		return 10;
	}else{
        printk("illegal system call %d\n", sys_num);
    }

    return 0;
}
