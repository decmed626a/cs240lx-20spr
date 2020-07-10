#ifndef __SINGLE_STEP_H__
#define __SINGLE_STEP_H__
#include "rpi.h"
#include "rpi-interrupts.h"
#include "rpi-constants.h"
#include "bit-support.h"

int fault_cnt;

// initialize single step mode: 1 on success.
int single_step_init(void);

// run a single routine <user_fn> in single step mode using stack <sp>
int single_step_run(int user_fn(void), uint32_t sp);

unsigned single_step_cnt(void) {
	return fault_cnt;
}

// assembly routine: run <fn> at user level with stack <sp>
//    XXX: visible here just so we can use it for testing.
int user_mode_run_fn(int (*fn)(void), uint32_t sp);

int run_fn_helper(uint32_t cpsr, void (*fn)(void), uint32_t sp);

int single_step_run(int user_fn(void), uint32_t sp) {
	return user_mode_run_fn(user_fn, sp);
}

int user_mode_run_fn(int (*fn)(void), uint32_t sp) {
	uint32_t new_cpsr = USER_MODE;
	return run_fn_helper(new_cpsr,(void (*) (void)) &fn, sp);
}

static void single_step_handler(uint32_t regs[16], uint32_t pc, uint32_t addr) {
	fault_cnt++;
	brkpt_mismatch_set0(pc, single_step_handler);
}

int single_step_init(void) {
	int retval = 0;
	fault_cnt = 0;
	cp14_enable();
	uint32_t cpsr_old = cpsr_get();
	brkpt_mismatch_set0(0, single_step_handler);
	retval = 1;
	return retval;
}

/*************************************************************
 * support code for tests.
 */
#endif
