#ifndef __SINGLE_STEP_H__
#define __SINGLE_STEP_H__

// initialize single step mode.
int single_step_init(void);

// run a single routine <user_fn> in single step mode using stack <sp>
int single_step_run(int user_fn(void), uint32_t sp);

static volatile int done = 0, start = 0;

int single_step_init(void) {
	cp14_enable();
	uint32_t cpsr_old = cpsr_get();
	brkpt_mismatch_set0(0, single_step_handler);
}

static void single_step_handler(uint32_t sp, uint32_t pc, uint32_t addr) {
	
}

int single_step_run(int user_fn(void), uint32_t sp) {
	user_trampoline_no_ret(mode_set(cpsr_old, USER_MODE), user_fn);
}

#endif
