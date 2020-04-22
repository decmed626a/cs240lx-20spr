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
    // implement this
    // use gpio_set0
    if (pin >= 32) {
        return;
    }
    // Get current register value
    volatile unsigned* gpio_reg = 0x0;
    unsigned bitmask = 0x0;
    
    if (pin <= 31){
        gpio_reg = GPSET0;
        // TODO: why do we not want this statement?
        //bitmask = get32(GPSET0);
    } else {
        gpio_reg = GPSET1;
        //bitmask = get32(GPSET1);
        // TODO: why do we not want this statement?
    }
    
    // Calculate bitmask to preserve values
    bitmask |= 1 << (pin % 32);
    
    put32(gpio_reg, bitmask);
}

// set GPIO <pin> off
static inline void fast_gpio_set_off(unsigned pin) {
    // implement this
    // use gpio_clr0
    if(pin >= 32) {
        return;
    }
    volatile unsigned* gpio_reg = 0x0;
    unsigned bitmask = 0x0;
    
    if (pin <= 31){
        gpio_reg = GPCLR0;
        //bitmask = get32(GPCLR0);
    } else {
        gpio_reg = GPCLR1;
        //bitmask = get32(GPCLR1);
    }
    
    // Calculate bitmask to preserve values
    bitmask |= 1 << (pin % 32);
    
    put32(gpio_reg, bitmask);
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
	static int count = 0;
	static int v = 0;
	unsigned start = cycle_cnt_read();
	unsigned first = start;

	// Unroll to 10 iterations
	// Hardcode set and clear
	// Get rid of v
	// Get rid of count
	
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
