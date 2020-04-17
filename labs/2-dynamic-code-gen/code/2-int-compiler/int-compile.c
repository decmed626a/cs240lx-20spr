#include "rpi.h"
#include "../unix-side/armv6-insts.h"

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#include "cycle-util.h"

typedef void (*int_fp)(void);

static volatile unsigned cnt = 0;

// fake little "interrupt" handlers: useful just for measurement.
void int_0() { printk("in 0\n"); cnt++; }
void int_1() { printk("in 1\n"); cnt++; }
void int_2() { printk("in 2\n"); cnt++; }
void int_3() { printk("in 3\n"); cnt++; }
void int_4() { printk("in 4\n"); cnt++; }
void int_5() { printk("in 5\n"); cnt++; }
void int_6() { printk("in 6\n"); cnt++; }
void int_7() { printk("in 7\n"); cnt++; }

static uint32_t code [256];
static unsigned m;

void int_compile(int_fp* intv, unsigned n) {
	m = 0;
	for(int i = 0; i < n; i++) {
		printk("iteration %d\n", i);
		code[m++] = arm_push(arm_r3);
		code[m++] = arm_push(arm_lr);
		code[m++] = arm_ldr_word_imm(arm_r3, arm_pc, 4);
		code[m] = arm_b3(1, (int)&code[m], (int)intv[i]);
		m++;
		code[m++] = arm_pop(arm_r3);
		code[m++] = arm_pop(arm_pc);
	}

	printk("emitted code: \n");
	for(int i = 0; i < n; i++) {
		printk("code[%d]=0x%x\n", i, code[i]);
	}
}


void generic_call_int(int_fp *intv, unsigned n) { 
    for(unsigned i = 0; i < n; i++)
        intv[i]();
}

// you will generate this dynamically.
void specialized_call_int(void) {
#if 0
	int_0();
    int_1();
    int_2();
    int_3();
    int_4();
    int_5();
    int_6();
    int_7();
#endif
	void (*fp)(void) = (typeof(fp))code;
	fp();
}

void notmain(void) {
    int_fp intv[] = {
        int_0,
        int_1,
        int_2,
        int_3,
        int_4,
        int_5,
        int_6,
        int_7
    };

    cycle_cnt_init();

    unsigned n = NELEM(intv);

	int_compile(intv, n);
	
    // try with and without cache: but if you modify the routines to do 
    // jump-threadig, must either:
    //  1. generate code when cache is off.
    //  2. invalidate cache before use.
    // enable_cache();

    cnt = 0;
    TIME_CYC_PRINT10("cost of generic-int calling",  generic_call_int(intv,n));
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    // rewrite to generate specialized caller dynamically.
    cnt = 0;
    TIME_CYC_PRINT10("cost of specialized int calling", specialized_call_int() );
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    clean_reboot();
}
