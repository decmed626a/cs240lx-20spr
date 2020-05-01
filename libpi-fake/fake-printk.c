#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int printk(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf("PI:");
	int res = vprintf(fmt, args);
	va_end(args);
	return res;
}
