#include "fake-pi.h"

unsigned timeout = 55000;

unsigned timer_get_usec(void)
{
	unsigned t = fake_time_inc(fake_random() % (timeout * 2));
	trace("getting usec = %dusec\n", t);
	return t;
}
