#include "fake-pi.h"

int mem_model_put32(volatile void *addr, uint32_t val) {
	return 0;
}
