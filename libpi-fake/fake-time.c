// simple fake time implementation.  add other time code you need.
#include "fake-pi.h"

unsigned fake_time_usec = 0;
// set the starting fake time.
void fake_time_init(unsigned init_time) {
    fake_time_usec = init_time;
}

void delay_us(unsigned us) {
	trace("delay_us = %dusec\n", us);
	fake_time_usec += us;
}
