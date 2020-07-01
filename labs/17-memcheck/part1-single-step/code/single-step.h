#ifndef __SINGLE_STEP_H__
#define __SINGLE_STEP_H__
#include "rpi.h"
#include "rpi-interrupts.h"
#include "rpi-constants.h"
#include "cp14-debug.h"
#include "bit-support.h"
#include "cpsr-util.h"

// initialize single step mode: 1 on success.
int single_step_init(void);

// run a single routine <user_fn> in single step mode using stack <sp>
int single_step_run(int user_fn(void), uint32_t sp);

// assembly routine: run <fn> at user level with stack <sp>
//    XXX: visible here just so we can use it for testing.
int user_mode_run_fn(int (*fn)(void), uint32_t sp);

int run_fn_helper(uint32_t cpsr, void (*fn)(void), uint32_t sp);

int user_mode_run_fn(int (*fn)(void), uint32_t sp) {
	uint32_t new_cpsr = USER_MODE;
	return run_fn_helper(new_cpsr,(void (*) (void)) &fn, sp);
}

/*************************************************************
 * support code for tests.
 */
static inline int mode_eq(uint32_t mode) { return mode_get(cpsr_get()) == mode; }
static inline int mode_is_super(void) { return mode_eq(SUPER_MODE); }
static inline int mode_is_user(void) { return mode_eq(USER_MODE); }

#endif
