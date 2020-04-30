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

static int rx = 20;
static int tx = 21;
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
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 7;
    while(cycle_cnt_read() - start < 6076 * (i + 1) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 6;
    while(cycle_cnt_read() - start < 6076 * (i + 2) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 5;
    while(cycle_cnt_read() - start < 6076 * (i + 3) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 4;
    while(cycle_cnt_read() - start < 6076 * (i + 4) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 3;
    while(cycle_cnt_read() - start < 6076 * (i + 5) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 2;
    while(cycle_cnt_read() - start < 6076 * (i + 6) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 1;
    while(cycle_cnt_read() - start < 6076 * (i + 7) + 3038) {}
    output |= (((*GPLEV0 & 0x100000) >> 20) & 1) << 0;
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

    while (((*GPLEV0 & 0x200000) >> 21) == 0) {
        ;
    }

	fast_gpio_set_off(pin);
	while(cycle_cnt_read() - start < 6076 * (i)) {}
	fast_gpio_write(pin, data & 1);
	while(cycle_cnt_read() - start < 6076 * (i + 1)) {}
	fast_gpio_write(pin, data & 2);
	while(cycle_cnt_read() - start < 6076 * (i + 2)) {}
	fast_gpio_write(pin, data & 4);
	while(cycle_cnt_read() - start < 6076 * (i + 3)) {}
	fast_gpio_write(pin, data & 8);
	while(cycle_cnt_read() - start < 6076 * (i + 4)) {}
	fast_gpio_write(pin, data & 16);
	while(cycle_cnt_read() - start < 6076 * (i + 5)) {}
	fast_gpio_write(pin, data & 32);
	while(cycle_cnt_read() - start < 6076 * (i + 6)) {}
	fast_gpio_write(pin, data & 64);
	while(cycle_cnt_read() - start < 6076 * (i + 7)) {}
	fast_gpio_write(pin, data & 128);
	while(cycle_cnt_read() - start < 6076 * (i + 8)) {}
	fast_gpio_set_on(pin);
	while(cycle_cnt_read() - start < 6076 * (i + 9)) {}
}

static void client(unsigned tx, unsigned rx, unsigned n) {
    printk("am a client \n");
	fast_gpio_set_on(tx);
    // we received 1 from server: next should be 0.
	unsigned curr_value = 0;
	for(int i = 0; i <= 4096; i++) {
		curr_value = scope(rx) << 24;
		curr_value |= scope(rx) << 16;
		curr_value |= scope(rx) << 8;
		curr_value |= scope(rx) << 0;
		//printk("RX: %d\n", curr_value);
		test_gen(tx, (curr_value & 0xFF000000) >> 24, 6076);
		//printk("TX1: %d\n", curr_value & 0xFF000000);
		test_gen(tx, (curr_value & 0x00FF0000) >> 16, 6076);
		//printk("TX2: %d\n", curr_value & 0x00FF0000);
		test_gen(tx, (curr_value & 0x0000FF00) >> 8, 6076);
		//printk("TX3: %d\n", curr_value & 0x0000FF00);
		test_gen(tx, (curr_value & 0x000000FF) >> 0, 6076);
    }
}

static volatile unsigned nevents = 0;
static volatile cycle_counter = 0;
// client has to define this.
void interrupt_vector(unsigned pc) {

    // you don't have to check anything else besides
    // if a gpio event was detected:
    //  - increment n_falling_edge if it was a falling edge
    //  - increment n_rising_edge if it was rising,
    // make sure you clear the GPIO event!
	unsigned s = cycle_cnt_read();
    dev_barrier();
    if(is_gpio_int(GPIO_INT0) || is_gpio_int(GPIO_INT1)) {
        if(gpio_read(rx) == 0) {
            n_falling_edge++;
			nevents++;
        } else if (gpio_read(rx) > 0) {
            n_rising_edge++;
			nevents++;
        }
        else {
            ;
        }
    }
	cycle_counter = s;
    gpio_event_clear(rx);
    dev_barrier();
}

void notmain() {
    uart_init();
    int_init();
    
	gpio_set_output(tx);
    gpio_set_input(rx);
    
	gpio_int_rising_edge(rx);
    gpio_int_falling_edge(rx);

	enable_cache();
    cycle_cnt_init();
    system_enable_interrupts();
    client(tx, rx, 4096);

    // starter code.
    // make sure this works first, then try to measure the overheads.

    clean_reboot();
}
