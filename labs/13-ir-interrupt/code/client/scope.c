/*
 * engler, cs140e: simple tests to check that you are handling rising and falling
 * edge interrupts correctly.
 *
 * NOTE: make sure you connect your GPIO 20 to your GPIO 21 (i.e., have it "loopback")
 */
#include "rpi.h"
#include "timer-interrupt.h"
#include "libc/circular.h"
#include "sw-uart.h"
#include "cycle-util.h"
#include "cs140e-src/cycle-count.h"
#include "../scope-constants.h"

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

static volatile int pin = 21;
volatile int n_rising_edge = 0;
volatile int n_falling_edge = 0;

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

volatile unsigned curr_value = 0;
volatile unsigned is_writing = 0;

volatile unsigned transition_buf[22];
volatile unsigned first_read = 0x200000;
volatile unsigned curr_read = 0;
volatile unsigned baseline_read = 0;

// dumb log.  use your own if you like!
typedef struct {
    unsigned v,ncycles;
} log_ent_t;

static int abs(int x) { return x < 0 ? -x : x; }

#   define MAXSAMPLES 22
static volatile log_ent_t log[MAXSAMPLES];

volatile unsigned num_transitions = 1;

static inline void
scope(unsigned pin, unsigned n_max, unsigned max_cycles) {
	// Just do one shift
	// Loop unrolling isn't super helpful..
	volatile unsigned start = cycle_cnt_read();
	unsigned cutoff = start + max_cycles;

	
	for(int i = 0; i < 300000; i++) {
		curr_read = *GPLEV0 & 0x200000; 
		if(baseline_read != curr_read){
			transition_buf[num_transitions] = cycle_cnt_read();
			baseline_read = curr_read;
			num_transitions++;
		}
		curr_read = *GPLEV0 & 0x200000; 
		if(baseline_read != curr_read){
			transition_buf[num_transitions] = cycle_cnt_read();
			baseline_read = curr_read;
			num_transitions++;
		}
		curr_read = *GPLEV0 & 0x200000; 
		if(baseline_read != curr_read){
			transition_buf[num_transitions] = cycle_cnt_read();
			baseline_read = curr_read;
			num_transitions++;
		}
		curr_read = *GPLEV0 & 0x200000; 
		if(baseline_read != curr_read){
			transition_buf[num_transitions] = cycle_cnt_read();
			baseline_read = curr_read;
			num_transitions++;
		}
		curr_read = *GPLEV0 & 0x200000; 
		if(baseline_read != curr_read){
			transition_buf[num_transitions] = cycle_cnt_read();
			baseline_read = curr_read;
			num_transitions++;
		}
		curr_read = *GPLEV0 & 0x200000; 
		if(baseline_read != curr_read){
			transition_buf[num_transitions] = cycle_cnt_read();
			baseline_read = curr_read;
			num_transitions++;
		}
		curr_read = *GPLEV0 & 0x200000; 
		if(baseline_read != curr_read){
			transition_buf[num_transitions] = cycle_cnt_read();
			baseline_read = curr_read;
			num_transitions++;
		}
		curr_read = *GPLEV0 & 0x200000; 
		if(baseline_read != curr_read){
			transition_buf[num_transitions] = cycle_cnt_read();
			baseline_read = curr_read;
			num_transitions++;
		}

	}
	unsigned pp_read = 1-((first_read & 1<<pin) >> pin);
	for(int a = 0; a < num_transitions; a++) {
		if(a == 0) {
			transition_buf[a] = start; 
		} 
	}


	for(int j = 0; j < num_transitions; j++) {
		
		log[j].v = pp_read;
		pp_read = 1-pp_read;
		log[j].ncycles = abs(transition_buf[j + 1] - transition_buf[j]);
	}

}

// dump out the log, calculating the error at each point,
// and the absolute cumulative error.
void dump_samples(unsigned n, unsigned period) {
    unsigned tot = 0, tot_err = 0;

    for(int i = 0; i < n-1; i++) {
        log_ent_t *e = &log[i];

        unsigned ncyc = e->ncycles;
        tot += ncyc;

        unsigned exp = period * (i+1);
        unsigned err = tot > exp ? tot - exp : exp - tot;
        tot_err += err;

        
		if(i == 4) {
            if(ncyc >= 400000 && ncyc <= 500000){
                printk("Power\n");
                return;
            }
        }

        if(i == 8) {
            if(ncyc >= 800000 && ncyc <= 900000){
                printk("Netflix\n");
                return;
            }
        }
        
        if(i == 12) {
            if(ncyc >= 800000 && ncyc <= 900000){
                printk("Subtitles\n");
                return;
            }
        }
	}
}


static volatile unsigned nevents = 0;
static volatile int cycle_counter = 0;
static volatile time_cycle = 0;
// client has to define this.
void interrupt_vector(unsigned pc) {
    // you don't have to check anything else besides
    // if a gpio event was detected:
    //  - increment n_falling_edge if it was a falling edge
    //  - increment n_rising_edge if it was rising,
    // make sure you clear the GPIO event!
    dev_barrier();
    // we received 1 from server: next should be 0.
    scope(pin, 20, time_cycle);
	gpio_event_clear(pin);
    dev_barrier();
}

void notmain() {
    uart_init();
	dev_barrier();
    int_init();
	system_enable_interrupts();
    dev_barrier();
    gpio_set_input(pin);
    gpio_int_falling_edge(pin);

	time_cycle = cycles_per_sec(1);

	enable_cache();
    cycle_cnt_init();

again:
	while(num_transitions == 1) {
		;
	}

	dump_samples(MAXSAMPLES, CYCLE_PER_FLIP);
    
	num_transitions = 1;
	for(int i = 0; i < 22; i++) {
		transition_buf[i] = 0;
	}
	
	for(int i = 0; i < 21; i++) {
		log[i].v = 0;
		log[i].ncycles = 0;
	}
	goto again;
	// starter code.
    // make sure this works first, then try to measure the overheads.

    clean_reboot();
}

