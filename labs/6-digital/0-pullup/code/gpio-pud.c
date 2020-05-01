// put your pull-up/pull-down implementations here.
#include "rpi.h"
#include "cs140e-src/cycle-count.h"

volatile unsigned *GPPUD = (void*) 0x20200094;
volatile unsigned *GPPUDCLK0 = (void*) 0x20200098;
volatile unsigned *GPPUDCLK1 = (void*) 0x2020009C;

void gpio_set_pullup(unsigned pin) { 
	unsigned val = (0x2); 
	put32(GPPUD, val);
	//printk("Wrote to GPPUD\n");
	unsigned start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}
	
	volatile unsigned* pud_clk_reg = 0;
	if(pin <=31) {
		pud_clk_reg = GPPUDCLK0;
	} else {
		pud_clk_reg = GPPUDCLK1;
	}
	//printk("Determined clock_reg\n");
	unsigned pud_offset = pin & 0x1f;
	put32(pud_clk_reg, (1 << pud_offset));
	//printk("Wrote clock reg\n");
	start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}
	
	put32(GPPUD, 0);
	//printk("cleared GPPUD\n");
	start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}

	put32(pud_clk_reg, 0);
	//printk("cleared clk reg\n");
	start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}
}
void gpio_set_pulldown(unsigned pin) {
	unsigned val = (0x1); 
	put32(GPPUD, val);
	//printk("Wrote to GPPUD\n");
	unsigned start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}
	
	volatile unsigned* pud_clk_reg = 0;
	if(pin <=31) {
		pud_clk_reg = GPPUDCLK0;
	} else {
		pud_clk_reg = GPPUDCLK1;
	}
	//printk("Determined clock_reg\n");
	unsigned pud_offset = pin & 0x1f;
	put32(pud_clk_reg, (1 << pud_offset));
	//printk("Wrote clock reg\n");
	start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}
	
	put32(GPPUD, 0);
	//printk("cleared GPPUD\n");
	start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}

	put32(pud_clk_reg, 0);
	//printk("cleared clk reg\n");
	start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}
}


void gpio_pud_off(unsigned pin) {
	unsigned val = (0x0); 
	put32(GPPUD, val);
	//printk("Wrote to GPPUD\n");
	unsigned start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}
	
	volatile unsigned* pud_clk_reg = 0;
	if(pin <=31) {
		pud_clk_reg = GPPUDCLK0;
	} else {
		pud_clk_reg = GPPUDCLK1;
	}
	//printk("Determined clock_reg\n");
	unsigned pud_offset = 0x0;
	put32(pud_clk_reg, pud_offset);
	//printk("Wrote clock reg\n");
	start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}
	
	put32(GPPUD, 0);
	//printk("cleared GPPUD\n");
	start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}

	put32(pud_clk_reg, 0);
	//printk("cleared clk reg\n");
	start = cycle_cnt_read();
	while(cycle_cnt_read() - start < 150) {}
}
