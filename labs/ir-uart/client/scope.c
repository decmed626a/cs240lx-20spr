// engler, cs240lx: simple sw_uart_get8 skeleton for logic analyzer.
#include "rpi.h"
#include "cs140e-src/cycle-count.h"

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

typedef struct {
    int tx,rx;
    unsigned baud;
    unsigned cycle_per_bit;  // usec we send each bit.
} my_sw_uart_t;

static  my_sw_uart_t u; 
// we inline compute usec_per_bit b/c we don't have division on the pi.  division
// by a constant allows the compiler to pre-compute.
#define sw_uart_init(_tx,_rx,_baud) \
    (my_sw_uart_t){ .tx = _tx, .rx = _rx, .baud = _baud, .cycle_per_bit = (700 * 1000*1000)/_baud }

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
sw_uart_get8(my_sw_uart_t* uart) {

    unsigned output = 0;
    unsigned i = 1;
    unsigned half_delay = uart->cycle_per_bit >> 1;
    while (((*GPLEV0 & (1 << uart->rx)) >> uart->rx) > 0) {
        ;
    }
    unsigned start  = cycle_cnt_read();
    while(cycle_cnt_read() - start < (uart->cycle_per_bit >> 1) * (i)) {}
    
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i) + half_delay) {}
    output |= (((*GPLEV0 & (1 << uart->rx)) >> uart->rx) & 1) << 0;
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 1) + half_delay) {}
    output |= (((*GPLEV0 & (1 << uart->rx)) >> uart->rx) & 1) << 1;
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 2) + half_delay) {}
    output |= (((*GPLEV0 & (1 << uart->rx)) >> uart->rx) & 1) << 2;
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 3) + half_delay) {}
    output |= (((*GPLEV0 & (1 << uart->rx)) >> uart->rx) & 1) << 3;
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 4) + half_delay) {}
    output |= (((*GPLEV0 & (1 << uart->rx)) >> uart->rx) & 1) << 4;
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 5) + half_delay) {}
    output |= (((*GPLEV0 & (1 << uart->rx)) >> uart->rx) & 1) << 5;
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 6) + half_delay) {}
    output |= (((*GPLEV0 & (1 << uart->rx)) >> uart->rx) & 1) << 6;
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 7) + half_delay) {}
    output |= (((*GPLEV0 & (1 << uart->rx)) >> uart->rx) & 1) << 7;
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 8) + half_delay) {}
   
	// sit and spin until the edge goes high, but needs timeout
	
	return output;
}

// send N samples at <ncycle> cycles each in a simple way.
void sw_uart_put8(my_sw_uart_t* uart, uint8_t data) {
    unsigned i = 1;
    unsigned start  = cycle_cnt_read();

    while (((*GPLEV0 & (1 << uart->tx)) >> uart->tx) == 0) {
        ;
    }

    fast_gpio_set_off(uart->tx);
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i)) {}
    fast_gpio_write(uart->tx, data & 128);
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 1)) {}
    fast_gpio_write(uart->tx, data & 64);
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 2)) {}
    fast_gpio_write(uart->tx, data & 32);
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 3)) {}
    fast_gpio_write(uart->tx, data & 16);
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 4)) {}
    fast_gpio_write(uart->tx, data & 8);
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 5)) {}
    fast_gpio_write(uart->tx, data & 4);
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 6)) {}
    fast_gpio_write(uart->tx, data & 2);
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 7)) {}
    fast_gpio_write(uart->tx, data & 1);
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 8)) {}
    fast_gpio_set_on(uart->tx);
    while(cycle_cnt_read() - start < uart->cycle_per_bit * (i + 9)) {}
}

static void client(unsigned tx, unsigned rx, unsigned n) {
    printk("am a client \n");
	fast_gpio_set_on(tx);
    // we received 1 from server: next should be 0.
	unsigned curr_value = 0;
	for(int i = 0; i <= 4096; i++) {
		curr_value = sw_uart_get8(&u) << 24;
		curr_value |= sw_uart_get8(&u) << 16;
		curr_value |= sw_uart_get8(&u) << 8;
		curr_value |= sw_uart_get8(&u) << 0;
		//printk("RX: %d\n", curr_value);
		sw_uart_put8(&u, (curr_value & 0xFF000000) >> 24);
		//printk("TX1: %d\n", curr_value & 0xFF000000);
		sw_uart_put8(&u, (curr_value & 0x00FF0000) >> 16);
		//printk("TX2: %d\n", curr_value & 0x00FF0000);
		sw_uart_put8(&u, (curr_value & 0x0000FF00) >> 8);
		//printk("TX3: %d\n", curr_value & 0x0000FF00);
		sw_uart_put8(&u, (curr_value & 0x000000FF) >> 0);
    }
}

void notmain(void) {
    int rx = 20;
    int tx = 21;
    u = sw_uart_init(tx, rx, 921600);
	gpio_set_output(tx);
    gpio_set_input(rx);
    enable_cache();
	cycle_cnt_init();

    client(tx, rx, 4096);
	
    clean_reboot();
}
