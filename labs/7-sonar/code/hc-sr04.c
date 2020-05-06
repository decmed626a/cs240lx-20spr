#include "rpi.h"
#include "hc-sr04.h"

volatile unsigned *GPPUD = (void*) 0x20200094;
volatile unsigned *GPPUDCLK0 = (void*) 0x20200098;
volatile unsigned *GPPUDCLK1 = (void*) 0x2020009C;

void gpio_set_pullup(unsigned pin) { 
	dev_barrier();
	put32(GPPUD, 0x2);
	delay_us(150);
	volatile unsigned* pud_clk_reg = 0;
	if(pin <=31) {
		pud_clk_reg = GPPUDCLK0;
	} else {
		pud_clk_reg = GPPUDCLK1;
	}
	unsigned pud_offset = pin & 0x1f;
	put32(pud_clk_reg, (1 << pud_offset));
	delay_us(150);
	put32(GPPUD, 0);
	delay_us(150);
	put32(pud_clk_reg, 0);
	delay_us(150);
	dev_barrier();
}
void gpio_set_pulldown(unsigned pin) {
	dev_barrier();
	put32(GPPUD, 0x1);
	delay_us(150);
	volatile unsigned* pud_clk_reg = 0;
	if(pin <=31) {
		pud_clk_reg = GPPUDCLK0;
	} else {
		pud_clk_reg = GPPUDCLK1;
	}
	unsigned pud_offset = pin & 0x1f;
	put32(pud_clk_reg, (1 << pud_offset));
	delay_us(150);
	put32(GPPUD, 0);
	delay_us(150);
	put32(pud_clk_reg, 0);
	delay_us(150);
	dev_barrier();
}



// gpio_read(pin) until either:
//  1. gpio_read(pin) != v ==> return 1.
//  2. <timeout> microseconds have passed ==> return 0
int read_while_eq(int pin, int v, unsigned timeout) {
	unsigned start = timer_get_usec();
	unsigned curr = 0;
	while(((gpio_read(pin) >> pin)) == v) {
		curr = timer_get_usec();
		if(curr  > start + timeout) {
			return 0;
		}
	}
	return (int) (curr - start);
}

// initialize:
//  1. setup the <trigger> and <echo> GPIO pins.
// 	2. init the HC-SR04 (pay attention to time delays here)
// 
// Pay attention to the voltages on:
//    - Vcc
//    - Vout.
//
// Troubleshooting:
// 	1. there are conflicting accounts of what value voltage you
//	need for Vcc.
//	
// 	2. the initial 3 page data sheet you'll find sucks; look for
// 	a longer one. 
//
// The comments on the sparkfun product page might be helpful.
hc_sr04_t hc_sr04_init(int trigger, int echo) {
    hc_sr04_t h = { .trigger = trigger, .echo = echo };
    gpio_set_output(trigger);
	gpio_set_input(echo);
	gpio_set_pulldown(echo);
	return h;
}

// get distance.
//	1. do a send (again, pay attention to any needed time 
// 	delays)
//
//	2. measure how long it takes and compute round trip
//	by converting that time to distance using the datasheet
// 	formula
//
// troubleshooting:
//  0. We don't have floating point or integer division.
//
//  1. The pulse can get lost!  Make sure you use the timeout read
//  routine you write.
// 
//	2. readings can be noisy --- you may need to require multiple
//	high (or low) readings before you decide to trust the 
// 	signal.
//
int hc_sr04_get_distance(hc_sr04_t *h, unsigned timeout_usec) {
	gpio_set_on(h->trigger);
	delay_us(10);
	gpio_set_off(h->trigger);
	read_while_eq(h->echo, 0, 200);
	int high_interval_us = read_while_eq(h->echo, 1, 55000);
    return high_interval_us;
}
