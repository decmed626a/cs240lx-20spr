#include "rpi.h"
#include "libc/helper-macros.h"
#include "ckalloc.h"

// simplistic heap management: a single, contiguous heap given to us by calling
// ck_init
static uint8_t *heap = 0, *heap_end, *heap_start;

void ck_init(void *start, unsigned n) {
    assert(aligned(heap_start, 8));
    printk("sizeof hdr=%d, redzone=%d\n", sizeof(hdr_t), REDZONE);
    heap = heap_start = start;
    heap_end = heap + n;
}

// simple access routines.

// pointer to first redzone
static void *b_rz1_ptr(hdr_t *h) { return (char *)h + sizeof *h; }
// address of user alloced space
static void *b_alloc_ptr(hdr_t *h) { return b_rz1_ptr(h) + REDZONE; }
// address of second redzone
static void *b_rz2_ptr(hdr_t *h) { return b_alloc_ptr(h) + h->nbytes_alloc; }
// includes the remainder
static unsigned b_rz2_nbytes(hdr_t *h) { return REDZONE + h->nbytes_rem; }

// given a pointer to the front of the allocated block, get the header.
static hdr_t *b_addr_to_hdr(void *addr) {
    return (void*) ((char *)addr - ALLOC_OFFSET);
}

// compute checksum on header.  need to do w/ cksum set to known value!
static uint32_t hdr_cksum(hdr_t *h) {
    unsigned old = h->cksum;
    h->cksum = 0;
    uint32_t cksum = fast_hash(h,sizeof *h);
    h->cksum = old;
    return cksum;
}

// check the header checksum and that its state == ALLOCED or FREED
static int check_hdr(hdr_t *h) {
	uint32_t hdr_cksum_val = hdr_cksum(h);
	printk("hdr_cksum_val = %x\n", hdr_cksum_val);
	printk("h->cksum = %x\n", h->cksum);
	assert(hdr_cksum_val == h->cksum);
	assert(h->state == ALLOCED || h->state == FREED);
	return 1;
}

static int check_mem(hdr_t *h, char *p, unsigned nbytes) {
    int i;
    for(i = 0; i < nbytes; i++) {
        if(p[i] != SENTINAL) {
			char* h_addr = b_alloc_ptr(h);
           	int offset = &p[i] - h_addr;
			ck_error(h, "block %p corrupted at offset %d\n", h_addr, offset); 
			return 0;
        }
    }
    return 1;
}
static void mark_mem(void *p, unsigned nbytes) {
    memset(p, SENTINAL, nbytes);
}

/*
 * check that:
 *  1. header is not corrupted (checksum passes).
 *  2. redzone 1 and redzone 2 are not corrupted.
 *
 */
static int check_block(hdr_t *h) {
    // short circuit the checks.
	return check_hdr(h)
        && check_mem(h, b_rz1_ptr(h), REDZONE)
        && check_mem(h, b_rz2_ptr(h), b_rz2_nbytes(h));
}

/*
 *  give an error if so.
 *  1. header is not in allocated state.
 *  2. allocated block does not pass checks.
 */
void (ckfree)(void *addr, const char *file, const char *func, unsigned lineno) {
    hdr_t *h = 0;
    demand(heap, not initialized?);
    trace("freeing %p\n", addr);
    
	h = b_addr_to_hdr(addr);
	
	if(!check_block(h))
		return;
	
	if(h->state == FREED) {
		return;
	}

	h->state = FREED;
	
	mark_mem(addr, h->nbytes_alloc);
	
	h->free_loc.file = file;
	h->free_loc.func = func;
	h->free_loc.lineno = lineno;

	h->cksum = hdr_cksum(h);
}

// check if nbytes + overhead causes an overflow.
void *(ckalloc)(uint32_t nbytes, const char *file, const char *func, unsigned lineno) {
    hdr_t *h = 0;
    void *ptr = 0;

    demand(heap, not initialized?);
    trace("allocating %d bytes\n", nbytes);

    unsigned tot = pi_roundup(nbytes, 8);
    unsigned n = tot + OVERHEAD_NBYTES;
    
    // this can overflow.
    if(n < nbytes)
        trace_panic("size overflowed: %d bytes is too large\n", nbytes);

    if((heap + n) >= heap_end)
        trace_panic("out of memory!  have only %d left, need %d\n", 
            heap_end - heap, n);
	
	h = (hdr_t*) heap;
	uint32_t total_size = ALLOC_OFFSET + n;
	
	h->nbytes_alloc = nbytes;
	h->nbytes_rem = tot - nbytes;
	h->state = ALLOCED;
	
	h->alloc_loc.file = file;
	h->alloc_loc.func = func;
	h->alloc_loc.lineno = lineno;

	h->free_loc.file = 0;
	h->free_loc.func = 0;
	h->free_loc.lineno = 0;
	
	h->cksum = hdr_cksum(h);

	mark_mem(b_rz1_ptr(h), REDZONE);
	mark_mem(b_rz2_ptr(h), b_rz2_nbytes(h));
	
	heap += n;
	ptr = b_alloc_ptr(h);

	assert(check_hdr(h));
    assert(check_block(h));
    trace("ckalloc:allocated %d bytes, (total=%d), ptr=%p\n", nbytes, n, ptr);
    return ptr;
}

// integrity check the allocated / freed blocks in the heap
// if the header of a block is corrupted, just return.
// return the error count.
int ck_heap_errors(void) {
    unsigned alloced = heap - heap_start;
    unsigned left = heap_end - heap;

    trace("going to check heap: %d bytes allocated, %d bytes left\n", 
            alloced, left);
    unsigned nerrors = 0;
    unsigned nblks = 0;

	uint8_t* check_ptr = heap_start;
	while(heap != check_ptr) {
		hdr_t* h = (void*)check_ptr;
		if(!check_hdr(h)) {
			trace("header is corrupted");
			nerrors++;
			break;
		}
		if(!check_block(h)) {nerrors++;}
		
		if(h->state == FREED) {
			if(!check_mem(h, b_alloc_ptr(h), h->nbytes_alloc) ) {
				trace("\tWrote block after free!\n");
				nerrors++;
			}
		}

		check_ptr += OVERHEAD_NBYTES + h->nbytes_alloc + h->nbytes_rem;
		nblks++;
	}

    
	if(nerrors)
        trace("checked %d blocks, detected %d errors\n", nblks, nerrors);
    else
        trace("SUCCESS: checked %d blocks, detected no errors\n", nblks);
    return nerrors;
}

