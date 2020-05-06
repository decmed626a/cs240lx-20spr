#include <stdio.h>
#include <stdlib.h>
#include "fake-pi.h"

void gpio_set_output(unsigned pin) {
	trace("pin=%d\n", pin);
}

void gpio_set_input(unsigned pin) {
	trace("pin=%d\n", pin);
}

void gpio_set_on(unsigned pin) {
	trace("pin=%d\n", pin);
}

void gpio_set_off(unsigned pin) {
	trace("pin=%d\n", pin);
}

void gpio_set_pulldown(unsigned pin) {
	trace("pin=%d\n", pin);
}

void gpio_write(unsigned pin, unsigned val) {
	trace("pin=%d, val=%d\n", pin, val);
}

int gpio_read(unsigned pin) {
	unsigned val = fake_random();
	trace("pin=%d, returning %d\n", pin, val);
	return val;
}
