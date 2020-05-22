#include "rpi.h"
#include "libc/helper-macros.h"
// #include "ckalloc.h"
#include "ckalloc-internal.h"

// simplistic heap management: a single, contiguous heap given to us by calling
// ck_init
static uint8_t *heap = 0, *heap_end, *heap_start;

static struct heap_info my_info;

void ck_init(void *start, unsigned n) {
    assert(aligned(heap_start, 8));
    printk("sizeof hdr=%d, redzone=%d\n", sizeof(hdr_t), REDZONE);
    heap = heap_start = start;
    heap_end = heap + n;
    my_info.heap_start = heap_start;
    my_info.heap_end = heap_start;
    my_info.nbytes_freed = 0;
    my_info.nbytes_alloced = 0;
}


// compute checksum on header.  need to do w/ cksum set to known value!
static uint32_t hdr_cksum(hdr_t *h) {
    unsigned old_cksum = h->cksum;
    unsigned old_refs_start = h->refs_start;
    unsigned old_refs_middle = h->refs_middle;
    unsigned old_mark = h->mark;
    h->cksum = 0;
    h->refs_start = 0;
    h->refs_middle = 0;
    h->mark = 0;
    uint32_t cksum = fast_hash(h,sizeof *h);
    h->cksum = old_cksum;
    h->refs_start = old_refs_start;
    h->refs_middle = old_refs_middle;
    h->mark = old_mark;
    return cksum;
}

// check the header checksum and that its state == ALLOCED or FREED
static int check_hdr(hdr_t *h) {
	uint32_t hdr_cksum_val = hdr_cksum(h);
	//printk("hdr_cksum_val = %x\n", hdr_cksum_val);
	//printk("h->cksum = %x\n", h->cksum);
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
			printk("p[i]: %x\n", p[i]);
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
 *  1. header is in allocated state.
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

	my_info.nbytes_freed += h->nbytes_alloc;

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
	heap += n;		// Update heap pointer immediately before writing to heap_info
	
	h->nbytes_alloc = nbytes;
	h->nbytes_rem = tot - nbytes;
	h->state = ALLOCED;

    my_info.nbytes_alloced += nbytes;
    my_info.heap_end = heap;

	h->alloc_loc.file = file;
	h->alloc_loc.func = func;
	h->alloc_loc.lineno = lineno;

	h->cksum = hdr_cksum(h);

	mark_mem(b_rz1_ptr(h), REDZONE);
	mark_mem(b_rz2_ptr(h), b_rz2_nbytes(h));
	
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
		assert(check_ptr < heap);
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


// returns pointer to the first header block.
// and checks the header
hdr_t *ck_first_hdr(void){
	hdr_t* curr_hdr = (hdr_t*)heap_start; 
	//printk("heap_start: %p\n", heap_start);
	//printk("curr_hdr: %p\n", curr_hdr);
	assert(check_hdr(curr_hdr));
	return curr_hdr;
}
// returns pointer to next hdr or 0 if none.
// and checks the header
hdr_t *ck_next_hdr(hdr_t *p){
	hdr_t* next_hdr = (void*)p + b_total_bytes(p);
	//printk("b_total_bytes: %x\n", b_total_bytes(p));
	//printk("curr_hdr: %p\n", p);
	//printk("next_hdr: %p\n", next_hdr);
	//printk("heap: %p\n", heap);
	//printk("heap_end: %p\n", heap_end);
	if((void*)next_hdr == (void*)heap) {
		return 0;
	}
	assert((uint8_t*)next_hdr < heap);
	assert(check_hdr(next_hdr));
	return next_hdr;
}

struct heap_info heap_info(void){
    return my_info;
}
