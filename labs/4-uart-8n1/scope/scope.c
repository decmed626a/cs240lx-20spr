// engler, cs240lx: simple scope skeleton for logic analyzer.
#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "../scope-constants.h"

#define DELAY 50

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

// dumb log.  use your own if you like!
typedef struct {
    unsigned v,ncycles;
} log_ent_t;

// return the value of <pin>
static inline int fast_gpio_read(unsigned pin) {
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
uint8_t
scope(unsigned pin) {


    int output = 0;
    int bit7 = 0;
    int bit6 = 0;
    int bit5 = 0;
    int bit4 = 0;
    int bit3 = 0;
    int bit2 = 0;
    int bit1 = 0;
    int bit0 = 0;
    
    unsigned i = 1;
    
    while (((*GPLEV0 & 0x100000)>> 20) > 0) {
        ;
    }
    unsigned start  = cycle_cnt_read();
    while(cycle_cnt_read() - start < 9114 * (i)) {}
    bit7 = (*GPLEV0 & 0x100000) >> 20;
    while(cycle_cnt_read() - start < 6076 * (i + 1)) {}
    bit6 = (*GPLEV0 & 0x100000) >> 20;
    while(cycle_cnt_read() - start < 6076 * (i + 2)) {}
    bit5 = (*GPLEV0 & 0x100000) >> 20;
    while(cycle_cnt_read() - start < 6076 * (i + 3)) {}
    bit4 = (*GPLEV0 & 0x100000) >> 20;
    while(cycle_cnt_read() - start < 6076 * (i + 4)) {}
    bit3 = (*GPLEV0 & 0x100000) >> 20;
    while(cycle_cnt_read() - start < 6076 * (i + 5)) {}
    bit2 = (*GPLEV0 & 0x100000) >> 20;
    while(cycle_cnt_read() - start < 6076 * (i + 6)) {}
    bit1 = (*GPLEV0 & 0x100000) >> 20;
    while(cycle_cnt_read() - start < 6076 * (i + 7)) {}
    bit0 = (*GPLEV0 & 0x100000) >> 20;
    while(cycle_cnt_read() - start < 6076 * (i + 8)) {}
    output = (bit7 & 1) | \
             (bit6 & 1) << 1 | \
             (bit5 & 1) << 2 | \
             (bit4 & 1) << 3 | \
             (bit3 & 1) << 4 | \
             (bit2 & 1) << 5 | \
             (bit1 & 1) << 6 | \
             (bit0 & 1) << 7;
    return output;
}

unsigned fuse_scope (unsigned pin) {
    unsigned fusion = 0;
    fusion |= scope(pin);
	//printk("Have %d\n", fusion);
	fusion |= scope(pin) << 8;
    //printk("Have %d\n", fusion);
    fusion |= scope(pin) << 16;
    //printk("Have %d\n", fusion);
    fusion |= scope(pin) << 24;
	//printk("Have %d\n", fusion);
    return fusion;
}


// send N samples at <ncycle> cycles each in a simple way.
void test_gen(unsigned pin, uint8_t data, unsigned ncycle) {
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

    fast_gpio_write(pin, 0);
    while(cycle_cnt_read() - start < ncycle * (i)) {}
    fast_gpio_write(pin, data & 1);
    while(cycle_cnt_read() - start < ncycle * (i + 1)) {}
    fast_gpio_write(pin, data & 2);
    while(cycle_cnt_read() - start < ncycle * (i + 2)) {}
    fast_gpio_write(pin, data & 4);
    while(cycle_cnt_read() - start < ncycle * (i + 3)) {}
    fast_gpio_write(pin, data & 8);
    while(cycle_cnt_read() - start < ncycle * (i + 4)) {}
    fast_gpio_write(pin, data & 16);
    while(cycle_cnt_read() - start < ncycle * (i + 5)) {}
    fast_gpio_write(pin, data & 32);
    while(cycle_cnt_read() - start < ncycle * (i + 6)) {}
    fast_gpio_write(pin, data & 64);
    while(cycle_cnt_read() - start < ncycle * (i + 7)) {}
    fast_gpio_write(pin, data & 128);
    while(cycle_cnt_read() - start < ncycle * (i + 8)) {}
    fast_gpio_write(pin, 1);
    while(cycle_cnt_read() - start < ncycle * (i + 9)) {}
    unsigned end = cycle_cnt_read();

    //printk("expected %d cycles, have %d\n", ncycle*10, end-start);
}

static void server(unsigned tx, unsigned rx, unsigned n) {
    
    printk("am a server, sending 0\n");
    unsigned curr_value = 0;
    unsigned expected = 1;

    test_gen(tx, curr_value, 6076);
    curr_value++;
    for(unsigned i = 0; i < n; i++) {
        // oh: have to wait.
        if(expected == scope(rx)) {
            curr_value = expected + 1;
            expected += 2;
            test_gen(tx, curr_value, 6076);
    	}
	}
	printk("server done: ended with %d\n", curr_value);
}

static void client(unsigned tx, unsigned rx, unsigned n) {
    printk("am a client\n");
	printk("waiting for 0\n");

    // we received 1 from server: next should be 0.
    unsigned reply = 0;
    unsigned expected = 0;
	unsigned temp = 0;
    while(reply < n) {
        //printk("%d: going to write: %d\n",i, v);
        if(expected == (temp = fuse_scope(rx))) {
			printk("Got %d\n", temp); 
            reply = expected + 1;
            expected += 2;
			fast_gpio_set_on(tx);
            //printk("Sending %d\n", (reply & 0xFF000000) >> 24);
			test_gen(tx, (reply & 0xFF000000) >> 24, 6076);
            //printk("Sending %d\n", (reply & 0x00FF0000) >> 16);
            test_gen(tx, (reply & 0x00FF0000) >> 16, 6076);
            //printk("Sending %d\n", (reply & 0x0000FF00) >> 8);
            test_gen(tx, (reply & 0x0000FF00) >> 8, 6076);
            //printk("Sending %d\n", (reply & 0x000000FF));
            test_gen(tx, (reply & 0x000000FF), 6076);
        }
    }
	printk("client done: ended with %d\n", reply-1);
}

void notmain(void) {
    int rx = 20;
    int tx = 21;
    gpio_set_output(tx);
    gpio_set_input(rx);
    enable_cache();
    cycle_cnt_init();

    if(!gpio_read(rx))
        server(tx, rx, 254);
    else
        client(tx, rx, 254);
    
    clean_reboot();
}
