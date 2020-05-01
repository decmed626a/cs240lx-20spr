// fake memory.
//
// for the moment: simply print what put32 is writing, and return a random value for
// get32
#include <stdlib.h>
#include <stdio.h>
#include "rpi.h"

#include "fake-pi.h"

volatile unsigned* TIMER_ADDRESS = (void*)0x20003004;


typedef struct {
	volatile void* addr;
	unsigned data;
} fake_mem_t;


//static fake_mem_t fake_mem[4096] = {{0}};
//static int fake_index = 0;

unsigned get32(const volatile void *addr) {
    // you won't need this if you write fake versions of the timer code.
    // however, if you link in the raw timer.c from rpi, thne you'll need
    // to recognize the time address and returns something sensible.
	unsigned val = 0;
	if(addr == TIMER_ADDRESS) {
		val = fake_time_inc(1);
	} else {
		val = fake_random();
	}
	trace("GET32(%p)=0x%x\n", addr, val);
	return val;
}

// for today: simply print (addr,val) using trace()
void put32(volatile void *addr, unsigned val) {
	//for(int i = 0; i < fake_index; i++) {
	//	if(addr == fake_mem[i].addr) {
	//		fake_mem[i].data = val;
			trace("PUT32(%p)=0x%x\n", addr, val);
	//		return;
	//	}
	//}
	
	//fake_mem[fake_index] = (fake_mem_t) {.addr = addr, .data = val};
	//fake_index++;
	return;
}
