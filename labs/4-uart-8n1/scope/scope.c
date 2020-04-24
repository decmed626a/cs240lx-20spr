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
unsigned 
scope(unsigned pin, log_ent_t *l, unsigned n_max, unsigned max_cycles) {


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

// dump out the log, calculating the error at each point,
// and the absolute cumulative error.
void dump_samples(log_ent_t *l, unsigned n, unsigned period) {
    unsigned tot = 0, tot_err = 0;

    for(int i = 0; i < n-1; i++) {
        log_ent_t *e = &l[i];

        unsigned ncyc = e->ncycles;
        tot += ncyc;

        unsigned exp = period * (i+1);
        unsigned err = tot > exp ? tot - exp : exp - tot;
        tot_err += err;

        printk(" %d: val=%d, time=%d, tot=%d: exp=%d (err=%d, toterr=%d)\n", i, e->v, ncyc, tot, exp, err, tot_err);
    }
}

void notmain(void) {
    int pin = 20;
	enable_cache();
    gpio_set_input(pin);
    cycle_cnt_init();

#   define MAXSAMPLES 10
    log_ent_t log[MAXSAMPLES];

    unsigned n = scope(pin, log, MAXSAMPLES, cycles_per_sec(1));

    // <CYCLE_PER_FLIP> is in ../scope-constants.h
    printk("UART got: %x\n", n);
    clean_reboot();
}
