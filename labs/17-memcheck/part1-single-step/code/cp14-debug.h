#ifndef __CP14_DEBUG_H__
#define __CP14_DEBUG_H__

/********************************************************************
 * co-processor helper macro (should move to main header).
 *
 * you can use it or not, depending on if it helps you.
 *
 * 
 * cp14 instructions have the form:
 *
 *  mrc p14, opcode_1, <Rd>, Crn, Crm, opcode_2
 * where 
 *  opcode_1 = 0, 
 *  crn = 0
 * and you only set opcode_2 and crm.
 *
 * so:
 *  mrc p14, 0, <Rd>, c0, Crm, opcode_2
 */

// turn <x> into a string
#define MK_STR(x) #x

// define a general co-processor inline assembly routine to set the value.
// from manual: must prefetch-flush after each set.
#define cp_asm_set(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)       \
    static inline void fn_name ## _set(uint32_t v) {                    \
        asm volatile ("mcr " MK_STR(coproc) ", "                        \
                             MK_STR(opcode_1) ", "                      \
                             "%0, "                                     \
                            MK_STR(Crn) ", "                            \
                            MK_STR(Crm) ", "                            \
                            MK_STR(opcode_2) :: "r" (v));               \
    }

#define cp_asm_get(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)       \
    static inline uint32_t fn_name ## _get(void) {                      \
        uint32_t ret;                                                   \
        asm volatile ("mrc " MK_STR(coproc) ", "                        \
                             MK_STR(opcode_1) ", "                      \
                             "%0, "                                     \
                            MK_STR(Crn) ", "                            \
                            MK_STR(Crm) ", "                            \
                            MK_STR(opcode_2) : "=r" (ret));             \
        return ret;                                                     \
    }


// specialize to cp14
#define cp14_asm_set(fn, crm, op2) cp_asm_set(fn, p14, 0, c0, crm, op2)
#define cp14_asm_get(fn, crm, op2) cp_asm_get(fn, p14, 0, c0, crm, op2)
// do both _set and _get
#define cp14_asm(fn,crm,op2) \
    cp14_asm_set(fn,crm,op2)   \
    cp14_asm_get(fn,crm,op2)

/********************************************************************
 * part 0: get debug id register
 */

struct debug_id {
    uint32_t    revision:4,     // 0:3  revision number
                variant:4,      // 4:7  major revision number
                :4,             // 8:11
                debug_rev:4,   // 12:15
                debug_ver:4,    // 16:19
                context:4,      // 20:23
                brp:4,          // 24:27 --- number of breakpoint register pairs
                                // add 1
                wrp:4          // 28:31 --- number of watchpoint pairs.
        ;
};

// 13-5
cp_asm_get(cp14_debug_id, p14, 0, c0, c0, 0);	

/********************************************************************
 * part 1: set a watchpoint on 0 (test2.c/test3.c)
 */
#include "bit-support.h"

//static inline uint32_t wfar_get(void) { unimplemented(); }
//static inline uint32_t dscr_get(void) { unimplemented(); }

cp_asm_get(wfar, p14, 0, c0, c6, 0);
//cp_asm_get(dscr, p14, 0, c0, c1, 0); 
//cp_asm_set(dscr, p14, 0, c0, c1, 0); 

// see 3-65 (page 198 in my arm1176.pdf)
//static inline uint32_t dfsr_get(void) {
//    unimplemented();
//}

cp_asm_get(dfsr, p15, 0, c5, c0, 0);
cp_asm_set(dfsr, p15, 0, c5, c0, 0);


cp_asm_get(far, p15, 0, c6, c0, 0);

// was watchpoint debug fault caused by a load?
static inline unsigned datafault_from_ld(void) {
	uint32_t dfsr_val = dfsr_get();
	return !((dfsr_val >> 11) & 0x1);
}

// ...  by a store?
static inline unsigned datafault_from_st(void) {
    return !datafault_from_ld();
}

static inline unsigned was_debugfault_r(uint32_t r) {
    // "these bits are not set on debug event.
    return ((r & 0b1111) == 0b0010);
}

// are we here b/c of a datafault?
static inline unsigned was_debug_datafault(void) {
    unsigned r = dfsr_get();
    if(!was_debugfault_r(r))
        panic("impossible: should only get datafaults\n");
    // 13-11: watchpoint occured: bits [5:2] == 0b0010
	return 1;
}

// 3-68: fault address register: hold the MVA that the fault occured at.
//static inline uint32_t far_get(void) {
//    unimplemented();
//}




// client supplied fault handler: give a pointer to the registers so 
// the client can modify them (for the moment pass NULL)
//  - <pc> is where the fault happened, 
//  - <addr> is the fault address.
typedef void (*handler_t)(uint32_t regs[16], uint32_t pc, uint32_t addr);

// set a watchpoint at <addr>: calls <handler> with a pointer to the registers.
void watchpt_set0(void *addr, handler_t watchpt_handle);

// enable the co-processor.
void cp14_enable(void);

/********************************************************************
 * part 2: set a breakpoint on 0 (test4.c) / foo (test5.c)
 */

cp_asm_set(ifar, p15, 0, c6, c0, 0);
cp_asm_get(ifar, p15, 0, c6, c0, 0);

// 3-66: instuction fault status register: hold source of last instruction
// fault.
//static inline uint32_t ifsr_get(void) {
//    unimplemented();
//}

cp_asm_set(ifsr, p15, 0, c5, c0, 1);

// 3-69: holds address of function that caused prefetch fault.
//static inline uint32_t ifar_get(void) {
//    unimplemented();
//}

cp_asm_get(ifsr, p15, 0, c5, c0, 1);

// was this a debug instruction fault?
static inline unsigned was_debug_instfault(void) {
    unsigned r = ifsr_get();
	return ((r & 0b1111) == 0b0010);
}

// set a breakpoint at <addr>: call handler when the fault happens.
void brkpt_set0(uint32_t addr, handler_t brkpt_handler);



/********************************************************************
 ********************************************************************
 * lab-16: single-stepping.
 */


/********************************************************************
 *
 * part 1: trivial single step.
 *  16-part1-test1. 16-part1-test2.c 16-part1-test3.c
 */

// simple debug macro: can turn it off/on by calling <brk_verbose({0,1})>
#define brk_debug(args...) if(brk_verbose_p) debug(args)

extern int brk_verbose_p;
static inline void brk_verbose(int on_p) { brk_verbose_p = on_p; }

// Set:
//  - cpsr to <cpsr>
//  - sp to <stack_addr>
// and then call <fp>.
//
// Does not return.
//
// used to get around the problem that if we switch to USER_MODE in C code,
// it will not have a stack pointer setup, which will cause all sorts of havoc.
void user_trampoline_no_ret(uint32_t cpsr, void (*fp)(void));

// set a mismatch on <addr> --- call <handler> on mismatch.
// NOTE:
//  - an easy way to mismatch the next instruction is to call with
//    use <addr>=0.
//  - you cannot get mismatches in "privileged" modes (all modes other than
//    USER_MODE)
//  - once you are in USER_MODE you cannot switch modes on your own since the
//    the required "msr" instruction will be ignored.  if you do want to
//    return from user space you'd have to do a system call ("swi") that switches.
void brkpt_mismatch_set0(uint32_t addr, handler_t handler);

/********************************************************************
 *
 * part 2: concurrency checking.
 *  16-part2-test1. 16-part2-test2.c 16-part2-test3.c
 */


// disable mismatch breakpoint <addr>: error if doesn't exist.
void brkpt_mismatch_disable0(uint32_t addr);

// get the saved status register.
static inline uint32_t spsr_get(void) {
    unimplemented();
}
// set the saved status register.
static inline void spsr_set(uint32_t spsr) {
    unimplemented();
}


// same as user_trampoline_no_ret except:
//  -  it returns to the caller with the cpsr set correctly.
//  - it calls <fp> with a user-supplied pointer.
//  16-part2-test1.c
void user_trampoline_ret(uint32_t cpsr, void (*fp)(void *handle), void *handle);

#endif