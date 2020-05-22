// starter code for trivial heap checking using interrupts.
#include "rpi.h"
#include "rpi-internal.h"
#include "ckalloc-internal.h"
#include "timer-interrupt.h"

static volatile unsigned rewritten_addresses [7] = {0};
static volatile unsigned target_encodings [7] = {0};
static volatile unsigned trampoline_code[5] = {0};
static volatile unsigned* code_array;
static volatile int address_index = 0;
// you'll need to pull your code from lab 2 here so you
// can fabricate jumps
// #include "armv6-insts.h"

// used to check initialization.
static volatile int init_p, check_on;
static volatile unsigned cnt, period, period_sum;

unsigned arm_AL = 0b1110;

static inline uint32_t arm_b3(uint32_t L, int32_t src, int32_t dest) {
	uint32_t inst = 0;
	int pc = src + 8;
	int offset = dest - pc;
	unsigned imm24 = offset;
	imm24 = imm24 >> 2;
	imm24 &= (~0U >> 8);
	uint32_t u = (arm_AL << 28) | (L << 24) | (0b101 << 25);
	return u | imm24;
}

static inline uint32_t arm_bl(uint32_t pc, uint32_t imm24) {
	uint32_t inst = 0;
	uint32_t value = imm24 & ~(0x20000000);
	value |= (0xFFFFFFFF - 0x2000000);
	value = (value << 2) + 2;
	inst = (0b1110) << 28;
	inst |= (0b101) << 25;
	inst |= 1 << 24;
	inst |= value & 0xFFFFFF;
	return inst;
}

void generate_trampoline_code(uint32_t pc) {
	trampoline_code[0] = 0xe92d5fff; // Push all the needed registers
	trampoline_code[1] = arm_bl(*(unsigned *)pc, ck_heap_errors());
	trampoline_code[2] = 0xe8bd5fff; // Pop all the needed register
	trampoline_code[3] = *(unsigned *)pc, 
	trampoline_code[4] = arm_b3(1, (int32_t)(&trampoline_code[4]), pc + 4);
}

void encode_target_instructions(void){
	// ldr r6, [pc, #136]
	/*
	target_encodings[0] = arm_ldr_word_imm(6, arm_pc, 136);
	target_encodings[1] = arm_ldr_word_imm(5, arm_pc, 136);
	target_encodings[2] = arm_str_byte_single(3, 4);
	target_encodings[3] = arm_ldr_word_imm(5, arm_pc, 76);
	target_encodings[4] = amr_ldr_word_imm(
	*/
	target_encodings[0] = 0xe59f6088;
	target_encodings[1] = 0xe59f5088;
	target_encodings[2] = 0xe5c43000;
	target_encodings[3] = 0xe59f504c;
	target_encodings[4] = 0xe59f001c;
	target_encodings[5] = 0xe59f100c;
	target_encodings[6] = 0xe59f200c;
}

// allow them to limit checking to a range.  for simplicity we 
// only check a single contiguous range of code.  initialize to 
// the entire program.
static uint32_t 
    start_check = (uint32_t)&__code_start__, 
    end_check = (uint32_t)&__code_end__,
    // you will have to define these functions.
    start_nocheck = (uint32_t)ckalloc_start,
    end_nocheck = (uint32_t)ckalloc_end;

static int in_range(uint32_t addr, uint32_t b, uint32_t e) {
    assert(b<e);
    return addr >= b && addr < e;
}

// if <pc> is in the range we want to check and not in the 
// range we cannot check, return 1.
int (ck_mem_checked_pc)(uint32_t pc) {
	if(!in_range(pc, start_nocheck, end_nocheck) && 
	   in_range(pc, start_check, end_check)) {
		return 1;
	}
	return 0;
}

// useful variables to track: how many times we did 
// checks, how many times we skipped them b/c <ck_mem_checked_pc>
// returned 0 (skipped)
static volatile unsigned checked = 0, skipped = 0;

unsigned ck_mem_stats(int clear_stats_p) { 
    unsigned s = skipped, c = checked, n = s+c;
    printk("total interrupts = %d, checked instructions = %d, skipped = %d\n",
        n,c,s);
    if(clear_stats_p)
        skipped = checked = 0;
    return c;
}

// note: lr = the pc that we were interrupted at.
// longer term: pass in the entire register bank so we can figure
// out more general questions.
void ck_mem_interrupt(uint32_t pc) {

    // we don't know what the user was doing.
    dev_barrier();

	if(ck_mem_checked_pc(pc) && check_on) {
		if(ck_heap_errors()) {
			panic("HEAP ERROR\n");
		}
		checked++;
		//ck_find_leaks(1);		
		//ck_gc();		
	} else {
		skipped++;
	}

    // we don't know what the user was doing.
    dev_barrier();
}

int check_if_rewritten(unsigned encoding) {
	for(int j = 0; j < 7; j++) {
		if(encoding == rewritten_addresses[j]) {
			return 1;	
		}
	}
	
	rewritten_addresses[address_index] = encoding;
	address_index++;
	return 0;
}


// note: lr = the pc that we were interrupted at.
// longer term: pass in the entire register bank so we can figure
// out more general questions.
void trampoline_mem_interrupt(uint32_t pc) {

    // we don't know what the user was doing.
    dev_barrier();

	for(int i = 0; i < 7; i++) {
		if(*(unsigned *)pc == target_encodings[i]) {
			if(check_if_rewritten(*(unsigned *)pc)) {
				break;
			} else {
				// go into trampoline
				generate_trampoline_code(pc);
				*(unsigned *)pc = arm_bl(pc, (uint32_t)trampoline_code);	
			}
		}
	}

	if(ck_mem_checked_pc(pc) && check_on) {
		if(ck_heap_errors()) {
			panic("HEAP ERROR\n");
		}
		checked++;
		//ck_find_leaks(1);		
		//ck_gc();		
	} else {
		skipped++;
	}

    // we don't know what the user was doing.
    dev_barrier();
}

// do any interrupt init you need, etc.
void ck_mem_init(void) { 
    assert(!init_p);
    init_p = 1;

    assert(in_range((uint32_t)ckalloc, start_nocheck, end_nocheck));
    assert(in_range((uint32_t)ckfree, start_nocheck, end_nocheck));
    assert(!in_range((uint32_t)printk, start_nocheck, end_nocheck));

	encode_target_instructions();
	code_array = kmalloc(&__code_end__ - &__code_start__);

	int_init();

	timer_interrupt_init(0x2);

	system_enable_interrupts();
}

// only check pc addresses [start,end)
void ck_mem_set_range(void *start, void *end) {
    assert(start < end);
	start_check = (uint32_t)start;
	end_check = (uint32_t)end;
}

// maybe should always do the heap check at the begining
void ck_mem_on(void) {
    assert(init_p && !check_on);
    check_on = 1;
	
	system_enable_interrupts();
}

// maybe should always do the heap check at the end.
void ck_mem_off(void) {
    assert(init_p && check_on);

    system_disable_interrupts();
	check_on = 0;
}

static inline uint32_t arm_ldr_word_imm(uint8_t rd, uint8_t rn, uint32_t offset_12) {
	
	uint32_t inst = 0;
	inst |= (0b1110) << 28;
	inst |= (0b0101) << 24;
	inst |= (0b1001) << 20;

	inst |= (rn & 0b1111) << 16;
	inst |= (rd & 0b1111) << 12;
	inst |= offset_12;

	return inst;
}

static inline uint32_t arm_str_byte_single(uint8_t rd, uint8_t rn) {
	
	uint32_t inst = 0;
	inst |= (0b1110) << 28;
	inst |= (0b0101) << 24;
	inst |= (0b1100) << 20;

	inst |= (rn & 0b1111) << 16;
	inst |= (rd & 0b1111) << 12;

	return inst;
}

// client has to define this.
void interrupt_vector(unsigned pc) {
    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);

    // if this isn't true, could be a GPU interrupt (as discussed in Broadcom):
    // just return.  [confusing, since we didn't enable!]
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;

    // Checkoff: add a check to make sure we have a timer interrupt
    // use p 113,114 of broadcom.

    /* 
     * Clear the ARM Timer interrupt - it's the only interrupt we have
     * enabled, so we don't have to work out which interrupt source
     * caused us to interrupt 
     *
     * Q: what happens, exactly, if we delete?
     */
    PUT32(arm_timer_IRQClear, 1);

    // asm volatile("stm %0, {lr}^" : "=r"(shadow_lr)); 

    /*
     * We do not know what the client code was doing: if it was touching a 
     * different device, then the broadcom doc states we need to have a
     * memory barrier.   NOTE: we have largely been living in sin and completely
     * ignoring this requirement for UART.   (And also the GPIO overall.)  
     * This is probably not a good idea and we should be more careful.
     */
    dev_barrier();    
    cnt++;

	trampoline_mem_interrupt((uint32_t) pc);
	//ck_mem_interrupt((uint32_t)pc);

    // compute time since the last interrupt.
    static unsigned last_clk = 0;
    unsigned clk = timer_get_usec();
    period = last_clk ? clk - last_clk : 0;
    period_sum += period;
    last_clk = clk;

    // Q: what happens (&why) if you uncomment the print statement?
    // printk("In interrupt handler at time: %d\n", clk);
}

