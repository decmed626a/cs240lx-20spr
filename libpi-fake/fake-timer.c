// simple fake time implementation.  add other time code you need.
#include "fake-pi.h"

void delay_us(unsigned us) {
	trace("delay_us = %dusec\n", us);
	fake_time_usec += us;
}

void delay_ms(unsigned ms) {
	trace("delay_ms = %dmsec\n", ms);
	fake_time_usec += 1;
}

unsigned timer_get_usec(void) {
	trace("getting usec = %dusec\n", fake_time_usec);
	return fake_time_usec;
}
