// fake memory.
//
// for the moment: simply print what put32 is writing, and return a random value for
// get32
#include <stdlib.h>
#include <stdio.h>
#include "rpi.h"

#include "fake-pi.h"

volatile unsigned* TIMER_ADDRESS = (void*)0x20003004;


unsigned get32(const volatile void *addr) {
    // you won't need this if you write fake versions of the timer code.
    // however, if you link in the raw timer.c from rpi, thne you'll need
    // to recognize the time address and returns something sensible.
	unsigned val = 0;
	if(!mem_model_get32(addr, &val)) {
		if(addr == TIMER_ADDRESS) {
			val = fake_time_inc(1);
		} else {
			val = fake_random();
		}
	}
	trace("GET32(%p)=0x%x\n", addr, val);
	return val;
}

// for today: simply print (addr,val) using trace()
void put32(volatile void *addr, unsigned val) {
	trace("PUT32(%p)=0x%x\n", addr, val);
	return;
}
