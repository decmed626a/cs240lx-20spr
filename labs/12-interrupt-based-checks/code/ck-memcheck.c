// starter code for trivial heap checking using interrupts.
#include "rpi.h"
#include "rpi-internal.h"
#include "ckalloc-internal.h"
#include "timer-interrupt.h"


// you'll need to pull your code from lab 2 here so you
// can fabricate jumps
// #include "armv6-insts.h"

// useful variables to track: how many times we did 
// checks, how many times we skipped them b/c <ck_mem_checked_pc>
// returned 0 (skipped)
static volatile unsigned checked = 0, skipped = 0;
static volatile uint32_t target_encodings[7] = {0};

// used to check initialization.
static volatile int init_p, check_on;
static volatile unsigned cnt, period, period_sum;
static volatile long p_counter = 0;

typedef struct {
	uint32_t encoding0;
	uint32_t encoding1;
	uint32_t encoding2;
	uint32_t encoding3;
	uint32_t encoding4;
}  trampoline_t;

static volatile trampoline_t* trampolines;
static volatile uint32_t* rewritten_instructions;
static volatile uint32_t* instructions = (void*) &__code_start__;

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

void foo(void) {
	//printk("hello there\n");
	checked++;
	if(ck_heap_errors()) {
		ck_mem_stats(1);
		panic("HEAP ERROR\n");
	}	
	ck_mem_stats(1);
}


void encode_target_instructions(void){
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
	} else {
		skipped++;
	}

    dev_barrier();
}

/*
int check_if_rewritten(unsigned pc) {
	for(int j = 0; j < 7; j++) {
		if(pc == rewritten_addresses[j]) {
			return 1;	
		}
	}
	
	rewritten_addresses[address_index] = pc;
	address_index++;
	return 0;
}
*/

int check_if_target(uint32_t pc) {
	for(int i = 0; i < 7; i++) {
		if(pc == target_encodings[i]) {
			return 1;
		}
	}
	return 0;
}

// note: lr = the pc that we were interrupted at.
// longer term: pass in the entire register bank so we can figure
// out more general questions.
void trampoline_mem_interrupt(uint32_t pc) {

	// Check if in interrupts

    // we don't know what the user was doing.
    dev_barrier();
	unsigned offset = (pc - (uint32_t)&__code_start__) / sizeof(uint32_t);
	if(ck_mem_checked_pc(pc) && !rewritten_instructions[offset]) {
		uint32_t original_instruction = instructions[offset];
		if(!check_if_target(*(unsigned *)pc)) {return;}	
		trampolines[offset] = (trampoline_t) {
			.encoding0 = 0xe92d5fff,
			.encoding1 = arm_b3(1, (int32_t)(&(trampolines[offset].encoding1)), (int)foo),
			.encoding2  = 0xe8bd5fff, // Pop all the needed register
			.encoding3 = original_instruction,
			.encoding4 = arm_b3(0, (int32_t)(&(trampolines[offset].encoding4)), pc + 4),
		};

		instructions[offset] = arm_b3(0, (uint32_t)(&(instructions[offset])),
									 (uint32_t)(&trampolines[offset]));
		rewritten_instructions[offset] = 1;
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
	trampolines = kmalloc(sizeof(trampoline_t) * (&__code_end__ - &__code_start__));
	rewritten_instructions = kmalloc(sizeof(uint8_t) * (&__code_end__ - &__code_start__));
	//instructions = kmalloc(sizeof(uint32_t) * (&__code_end__ - &__code_start__));

	int_init();

	timer_interrupt_init(0x1);

	system_enable_interrupts();
}

// only check pc addresses [start,end)
void ck_mem_set_range(void *start, void *end) {
    assert(start < end);
	start_check = (uint32_t)start;
	end_check = (uint32_t)end;
	// May need to reset code_array to 0 
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

