// simple fake time implementation.  add other time code you need.
#include "fake-pi.h"

void delay_us(unsigned us) {
	trace("delay_us = %dusec\n", us);
	fake_time_usec += us;
}

void delay_ms(unsigned ms) {
	trace("delay_ms = %dms\n", ms);
	fake_time_usec += 1000 * ms;
}

