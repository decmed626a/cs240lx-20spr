#include "rpi.h"
#include "../unix-side/armv6-insts.h"

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#include "cycle-util.h"

typedef void (*int_fp)(void);

static volatile unsigned cnt = 0;

// fake little "interrupt" handlers: useful just for measurement.
void int_0() { cnt++; }
void int_1() { cnt++; }
void int_2() { cnt++; }
void int_3() { cnt++; }
void int_4() { cnt++; }
void int_5() { cnt++; }
void int_6() { cnt++; }
void int_7() { cnt++; }

static uint32_t code [128] __attribute__((aligned(16)));
static uint8_t m;

void int_compile(int_fp* intv, unsigned n) {

	m = 0;
	code[m++] = arm_push(arm_lr);
	for(int i = 0; i < n; i++) {
		code[m] = arm_b3(1, (int)&code[m], (int)intv[i]);
		m++;
	}
	code[m++] = arm_pop(arm_pc);

	/*
	printk("emitted code: \n");
	for(int i = 0; i < m; i++) {
		printk("code[%d]=0x%x\n", i, code[i]);
	}
	*/
}

static inline void* get_pc(void)  {
    void *pc;
    asm("mov %0, pc" : "=r"(pc));
    return pc;
}

void jump_thread_compile(int_fp* intv, unsigned n) {
	
	uint32_t bx_lr_code = arm_bx(arm_lr);
	
	for(int i = 0; i < n; i++) {
		int_fp* curr_inst = (int_fp*)intv[i];
		while(memcmp((void*)(&bx_lr_code), (void*)(curr_inst), sizeof(uint32_t)) != 0) {
			curr_inst++;
		}
		
		if(i != n-1) {
			uint32_t new_inst = arm_ldr_word_imm(0, arm_pc, intv[i+1]);
			*curr_inst = new_inst;
		}
	}

}

void generic_call_int(int_fp *intv, unsigned n) { 
    for(unsigned i = 0; i < n; i++)
        intv[i]();
}

// you will generate this dynamically.
void specialized_call_int(void) {
	int_0();
    int_1();
    int_2();
    int_3();
    int_4();
    int_5();
    int_6();
    int_7();
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
    
	cnt = 0;
	void (*dynamic_call_int)(void) = (typeof(dynamic_call_int))code;
    TIME_CYC_PRINT10("cost of dynamic int calling", dynamic_call_int() );
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

	jump_thread_compile(intv, n);
	
	cnt = 0;
    TIME_CYC_PRINT10("cost of jump threading", int_0() );
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);
    
	clean_reboot();
}

