#include <stdio.h>
#include <stdlib.h>
#include "fake-pi.h"

void gpio_set_input(unsigned pin) {
	trace("pin=%d\n", pin);
}
