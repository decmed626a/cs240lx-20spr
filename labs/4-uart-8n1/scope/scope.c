// engler, cs240lx: simple scope skeleton for logic analyzer.
#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "../scope-constants.h"

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

// return the value of <pin>
static inline unsigned fast_gpio_read(unsigned pin) {
    return (*GPLEV0);
}

// compute the number of cycles per second
unsigned cycles_per_sec(unsigned s) {
    demand(s < 2, will overflow);
	unsigned first = cycle_cnt_read();
	delay_ms(1000 * s);
	unsigned last = cycle_cnt_read();
	return last-first;
}

// monitor <pin>, recording any transitions until either:
//  1. we have run for about <max_cycles>.  
//  2. we have recorded <n_max> samples.
//
// return value: the number of samples recorded.
unsigned 
scope(unsigned pin) {

    unsigned output = 0;
    unsigned i = 1;
    while (((*GPLEV0 & 0x100000)>> 20) > 0) {
        ;
    }
    unsigned start  = cycle_cnt_read();
    while(cycle_cnt_read() - start < 3038 * (i)) {}
    
    while(cycle_cnt_read() - start < 6076 * (i) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 0;
    while(cycle_cnt_read() - start < 6076 * (i + 1) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 1;
    while(cycle_cnt_read() - start < 6076 * (i + 2) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 2;
    while(cycle_cnt_read() - start < 6076 * (i + 3) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 3;
    while(cycle_cnt_read() - start < 6076 * (i + 4) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 4;
    while(cycle_cnt_read() - start < 6076 * (i + 5) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 5;
    while(cycle_cnt_read() - start < 6076 * (i + 6) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 6;
    while(cycle_cnt_read() - start < 6076 * (i + 7) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 7;
    while(cycle_cnt_read() - start < 6076 * (i + 8) + 3038) {}
    /*
	output = (bit7 & 1) | \
             (bit6 & 1) << 1 | \
             (bit5 & 1) << 2 | \
             (bit4 & 1) << 3 | \
             (bit3 & 1) << 4 | \
             (bit2 & 1) << 5 | \
             (bit1 & 1) << 6 | \
             (bit0 & 1) << 7;
    */
	return output;
}

// send N samples at <ncycle> cycles each in a simple way.
void test_gen(unsigned pin, uint8_t data, unsigned ncycle) {
    unsigned i = 1;
    unsigned start  = cycle_cnt_read();

    fast_gpio_write(pin, 0);
    while(cycle_cnt_read() - start < 6076 * (i)) {}
    fast_gpio_write(pin, data & 128);
    while(cycle_cnt_read() - start < 6076 * (i + 1)) {}
    fast_gpio_write(pin, data & 64);
    while(cycle_cnt_read() - start < 6076 * (i + 2)) {}
    fast_gpio_write(pin, data & 32);
    while(cycle_cnt_read() - start < 6076 * (i + 3)) {}
    fast_gpio_write(pin, data & 16);
    while(cycle_cnt_read() - start < 6076 * (i + 4)) {}
    fast_gpio_write(pin, data & 8);
    while(cycle_cnt_read() - start < 6076 * (i + 5)) {}
    fast_gpio_write(pin, data & 4);
    while(cycle_cnt_read() - start < 6076 * (i + 6)) {}
    fast_gpio_write(pin, data & 2);
    while(cycle_cnt_read() - start < 6076 * (i + 7)) {}
    fast_gpio_write(pin, data & 1);
    while(cycle_cnt_read() - start < 6076 * (i + 8)) {}
    fast_gpio_write(pin, 1);
    while(cycle_cnt_read() - start < 6076 * (i + 9)) {}

}

static void client(unsigned tx, unsigned rx, unsigned n) {
    printk("am a client \n");
	fast_gpio_set_on(tx);
    // we received 1 from server: next should be 0.
	unsigned curr_value = 0;
	for(int i = 0; i <= 255; i++) {
		curr_value = scope(rx);
		test_gen(tx, curr_value, 6076);
    }
}

void notmain(void) {
    int rx = 20;
    int tx = 21;
    gpio_set_output(tx);
    gpio_set_input(rx);
    enable_cache();
	cycle_cnt_init();

    client(tx, rx, 255);
	
    clean_reboot();
}
