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
static cq_t putQ;
volatile int n_rising_edge = 0;
volatile int n_falling_edge = 0;

typedef struct {
    int tx,rx;
    unsigned baud;
    unsigned cycle_per_bit;  // usec we send each bit.
} my_sw_uart_t;

static  my_sw_uart_t u;

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
static inline unsigned
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
static inline void sw_uart_put8(my_sw_uart_t* uart, uint8_t data) {
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

volatile unsigned curr_value = 0;
volatile unsigned is_writing = 0;
static void client(unsigned tx, unsigned rx, unsigned n) {
    printk("am a client \n");
	system_enable_interrupts();
	fast_gpio_set_on(tx);
    // we received 1 from server: next should be 0.
	while(curr_value < n) {
		while(!cq_empty(&putQ)) {
			curr_value = cq_pop(&putQ);
			system_disable_interrupts();
            sw_uart_put8(&u, curr_value);
			system_enable_interrupts();
		}
	}
}

static volatile unsigned nevents = 0;
static volatile int cycle_counter = 0;
// client has to define this.
void interrupt_vector(unsigned pc) {

    // you don't have to check anything else besides
    // if a gpio event was detected:
    //  - increment n_falling_edge if it was a falling edge
    //  - increment n_rising_edge if it was rising,
    // make sure you clear the GPIO event!
    dev_barrier();
	cq_push(&putQ, sw_uart_get8(&u));
    gpio_event_clear(rx);
    dev_barrier();
}

void notmain() {
    uart_init();
    int_init();
    
	gpio_set_output(tx);
    gpio_set_input(rx);
    u = sw_uart_init(tx, rx, 230400);
    gpio_int_falling_edge(rx);
	cq_init(&putQ, 1);

	enable_cache();
    cycle_cnt_init();
    //system_enable_interrupts();
    client(tx, rx, 255);

    // starter code.
    // make sure this works first, then try to measure the overheads.

    clean_reboot();
}
