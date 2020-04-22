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
	unsigned num_samples = 0;
	unsigned curr_read;
	unsigned baseline_read;
    log_ent_t temp[10];
	
	unsigned first_read = (fast_gpio_read(pin));
	
	// Just do one shift
	// Loop unrolling isn't super helpful..
	while(first_read == (baseline_read=*GPLEV0)) {}
	unsigned start = cycle_cnt_read();

	// Rare that we run out of max cycles and num samples; don't care if we overshoot :) 
	// Enqueue items and then check at the end if we fail
	// Have a hard loop in the center: to reduce number of checks
	// Replace get32 and put32 with volatile pointer accesses
	// Replace array notation with pointer notation 
#if 0
	while(num_cycles <= max_cycles && num_samples <= n_max) {
		unsigned curr_read = fast_gpio_read(pin) >> pin;
		if(baseline_read != curr_read){
			baseline_read = curr_read;
			l[num_samples].v = curr_read;
			l[num_samples].ncycles = cycle_cnt_read() - start;
			start = cycle_cnt_read();
			num_samples++;
		}
	}
#endif 

	for(int i = 0; i < 10000; i++) {
		curr_read = *GPLEV0; 
		if(baseline_read != curr_read){
			temp[num_samples].v = curr_read;
			temp[num_samples].ncycles = cycle_cnt_read();
			baseline_read = curr_read;
			num_samples++;
		}
	}

	for(int a = 0; a <= num_samples; a++) {
		temp[a].v= (temp[a].v & 1<<pin) >> pin;
		temp[a].ncycles = temp[a].ncycles - start;
	}

	for(int j = 0; j <= num_samples; j++) {
		l[j].v = temp[j].v;
		l[j].ncycles = temp[j + 1].ncycles - temp[j].ncycles;
	}

	printk("First read: %d\n", (first_read & 1<<pin)>>pin);
	printk("Num samples: %d\n", num_samples);
	return num_samples;
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
    int pin = 21;
	enable_cache();
    gpio_set_input(pin);
    cycle_cnt_init();

#   define MAXSAMPLES 10
    log_ent_t log[MAXSAMPLES];

    unsigned n = scope(pin, log, MAXSAMPLES, cycles_per_sec(1));

    // <CYCLE_PER_FLIP> is in ../scope-constants.h
    dump_samples(log, n, CYCLE_PER_FLIP);
    clean_reboot();
}
