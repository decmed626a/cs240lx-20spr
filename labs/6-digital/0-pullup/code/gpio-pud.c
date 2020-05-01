// put your pull-up/pull-down implementations here.
#include "rpi.h"
#include "cs140e-src/cycle-count.h"

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


void gpio_pud_off(unsigned pin) {
	dev_barrier();
	put32(GPPUD, 0x0);
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
