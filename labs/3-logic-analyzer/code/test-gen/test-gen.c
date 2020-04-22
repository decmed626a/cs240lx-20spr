// engler, cs240lx: skeleton for test generation.  
#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/cycle-util.h"

// see broadcomm documents for magic addresses.
#define GPIO_BASE 0x20200000
static volatile unsigned *gpio_fsel0 = (void*)(GPIO_BASE + 0x00);
static volatile unsigned *gpio_set0  = (void*)(GPIO_BASE + 0x1C);
static volatile unsigned *gpio_clr0  = (void*)(GPIO_BASE + 0x28);

static volatile unsigned *GPFSEL0 = (void*) 0x20200000;
static volatile unsigned *GPFSEL1 = (void*) 0x20200004;
static volatile unsigned *GPFSEL2 = (void*) 0x20200008;
static volatile unsigned *GPFSEL3 = (void*) 0x2020000C;
static volatile unsigned *GPFSEL4 = (void*) 0x20200010;
static volatile unsigned *GPFSEL5 = (void*) 0x20200014;

static volatile unsigned *GPSET0 = (void*) 0x2020001C;
static volatile unsigned *GPSET1 = (void*) 0x20200020;

static volatile unsigned *GPCLR0 = (void*) 0x20200028;
static volatile unsigned *GPCLR1 = (void*) 0x2020002C;

static volatile unsigned *GPLEV0 = (void*) 0x20200034;
static volatile unsigned *GPLEV1 = (void*) 0x20200038;

#include "../scope-constants.h"
// set GPIO <pin> on.
static inline void fast_gpio_set_on(unsigned pin) {
    *GPSET0 |= 1 << (pin);
}

// set GPIO <pin> off
static inline void fast_gpio_set_off(unsigned pin) {
    *GPCLR0 |= 1 << (pin);
}

// set <pin> to <v> (v \in {0,1})
static inline void fast_gpio_write(unsigned pin, unsigned v) {
    
    if(v)
        fast_gpio_set_on(pin);
    else
        fast_gpio_set_off(pin);
}
// send N samples at <ncycle> cycles each in a simple way.
void test_gen(unsigned pin, unsigned N, unsigned ncycle) {
	unsigned i = 1;
	unsigned start = cycle_cnt_read();

	// Unroll to 10 iterations
	// Hardcode set and clear
	// Get rid of v
	// Get rid of count
	
# if 0
	while(count < N) {
		unsigned sample = cycle_cnt_read();
		if(sample - first >= ncycle) {
			fast_gpio_write(pin, 1-v);
			first = sample; // This error is additive :( Just use start instead
			// whiel cycle_cnt_read - start < ncycles * iteration
			count++;
			v ^= 1;
		}
	}
#endif

	fast_gpio_set_on(pin);
	while(cycle_cnt_read() - start < ncycle * (i)) {}
	fast_gpio_set_off(pin);
	while(cycle_cnt_read() - start < ncycle * (i + 1)) {}
	fast_gpio_set_on(pin);
	while(cycle_cnt_read() - start < ncycle * (i + 2)) {}
	fast_gpio_set_off(pin);
	while(cycle_cnt_read() - start < ncycle * (i + 3)) {}
	fast_gpio_set_on(pin);
	while(cycle_cnt_read() - start < ncycle * (i + 4)) {}
	fast_gpio_set_off(pin);
	while(cycle_cnt_read() - start < ncycle * (i + 5)) {}
	fast_gpio_set_on(pin);
	while(cycle_cnt_read() - start < ncycle * (i + 6)) {}
	fast_gpio_set_off(pin);
	while(cycle_cnt_read() - start < ncycle * (i + 7)) {}
	fast_gpio_set_on(pin);
	while(cycle_cnt_read() - start < ncycle * (i + 8)) {}
	fast_gpio_set_off(pin);
	while(cycle_cnt_read() - start < ncycle * (i + 9)) {}
	fast_gpio_set_on(pin);
	while(cycle_cnt_read() - start < ncycle * (i + 10)) {}
    unsigned end = cycle_cnt_read();

    // crude check how accurate we were ourselves.
    printk("expected %d cycles, have %d\n", ncycle*N, end-start);
}

void notmain(void) {
    int pin = 21;
	enable_cache();
    gpio_set_output(pin);
    cycle_cnt_init();

    // keep it seperate so easy to look at assembly.
    test_gen(pin, 11, CYCLE_PER_FLIP);

    clean_reboot();
}
