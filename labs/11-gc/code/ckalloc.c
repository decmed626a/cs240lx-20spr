#include "rpi.h"
#include "libc/helper-macros.h"
// #include "ckalloc.h"
#include "ckalloc-internal.h"

// simplistic heap management: a single, contiguous heap given to us by calling
// ck_init
static uint8_t *heap = 0, *heap_end, *heap_start;

static struct heap_info my_info;

void ck_init(void *start, unsigned n) {
    assert(aligned(start, 8));
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
    if(hdr_cksum(h) != h->cksum){
        return 0;
    }
    if(h->state != ALLOCED && h->state != FREED){
        return 0;
    }
    return 1;
}

// ugly; just pass h... definitely not offset
static int check_mem(char *p, unsigned nbytes, int offset, hdr_t * h) {
    int i;
    for(i = 0; i < nbytes; i++) {
        if(p[i] != SENTINAL) {
            ck_error(h, "block %p corrupted at offset %d\n", b_alloc_ptr(h), i + offset);
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
    unsigned result =  check_hdr(h)
        && check_mem(b_rz1_ptr(h), REDZONE, -REDZONE, h)
        && check_mem(b_rz2_ptr(h), b_rz2_nbytes(h), h->nbytes_alloc, h);
    return result;
    // if(!result){
    //     hdr_print(h);
    // }

    //return 1;
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

    // check that everything is ok
    check_block(h); // dawson doesn't assert; we just want to continue

    assert(h->state == ALLOCED);

    // mark as freed
    h->state = FREED;
    h->free_loc = (src_loc_t) {.file = file, .func = func, .lineno = lineno};
    // h->nbytes_rem += h->nbytes_alloc; // ?... perhaps we shouldn't do this either, since the below line isn't
    // h->nbytes_alloc = 0; // ?. no dawson doesn't have
    mark_mem(addr, h->nbytes_rem + h->nbytes_alloc);
    // h-> alloc_loc = none; // ?
    my_info.nbytes_freed += h->nbytes_alloc;
    h->cksum = hdr_cksum(h);
    // check again
    // assert(check_block(h));
    // if(!check_block(h)){
    //     hdr_print(h);
    // }
    // assert(check_hdr(h));
    // assert(check_mem(b_rz1_ptr(h), REDZONE));
    // assert(check_mem(b_rz2_ptr(h), b_rz2_nbytes(h)));

    //? should we do more stuff here to clear / update memory?
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
    
    // header
    h = (hdr_t * ) heap;
    heap += n;
    h->nbytes_alloc = nbytes;
    my_info.nbytes_alloced += nbytes;
    my_info.heap_end = heap; // this can probably be removed
    h->nbytes_rem = tot - nbytes; // I think
    h->state = ALLOCED;
    h->alloc_loc = (src_loc_t) {.file = file, .func = func, .lineno = lineno};
    h->cksum = hdr_cksum(h);

    // redzone 1
    mark_mem(b_rz1_ptr(h), REDZONE);

    // data 
    ptr = b_alloc_ptr(h);

    // redzone 2
    mark_mem(b_rz2_ptr(h), b_rz2_nbytes(h));

    assert(check_hdr(h));
    assert(check_block(h));

    trace("ckalloc:allocated %d bytes, (total=%d), ptr=%p\n", nbytes, n, ptr);
    return ptr;
}

// checks the data i.e. if freed, should be red zone
int check_data(hdr_t * h){
    if(h->state == FREED){
        return check_mem(b_alloc_ptr(h), h->nbytes_alloc, 0, h);
    }
    return 1;
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
    // unimplemented();

    uint8_t * curr_h = heap_start;
    while(curr_h != heap){
        hdr_t * h = (hdr_t *) curr_h;
        nblks++;
        if(!check_hdr(h)){
            printk("ALEX: HEADER OFF\n");
            return 1;
        }
        if(!check_block(h)){
            printk("ALEX: BLOCK OFF\n");
            nerrors++;
        }
        // check data
        if(!check_data(h)){
            trace("\tWrote block after free!\n");
            nerrors++;
        }

        curr_h = curr_h + OVERHEAD_NBYTES + h->nbytes_alloc + h->nbytes_rem;
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
    hdr_t * first = (hdr_t *) heap_start;
    check_hdr(first);
    return first;
}
// returns pointer to next hdr or 0 if none.
// and checks the header
hdr_t *ck_next_hdr(hdr_t *p){
    unsigned n = 0;
    n += pi_roundup(p->nbytes_alloc, 8);
    n += OVERHEAD_NBYTES;
    hdr_t * next = (hdr_t *) ((uint8_t *)p + n);
    if((uint8_t *) next == heap){
        return 0;
    }
    if(!check_hdr(next)){
    //    panic("check next header failed"); 
    }
    return next;
}

struct heap_info heap_info(void){
    return my_info;
}
/** 
 * garbage collection
 * 
 * set up shadow memory
 * 
 * in hdr_t we have # references to ptr
 * 
 * when we get a ptr, we want to know if it points to something allocated
 * 
 * could traverse over heap
 * 
 * or shadow mem of heap, and for each word in shadow mem mark which block (in heap) it corresponds to
 * (you write address points to it or something, and if not allocated, then null)
 * 
 * start with regs, stack, bss, data, and mark allocated memory. then anything allocated that's potentially pointed to
 * on heap is 
 * 
 * shadow memory: (not needed, but makes things faster) but when walking over heap and find something that could be a ptr, we need to look at which block it pts to, and we could do a 
 * linear walk over heap, or we could use shadow memory to figure out which block it points to. 
 * where to get memroy for shadow mem? Use kmalloc, or malloc or smth.
 * 
 * 
 * not: where we start from registers and do a DFS (this is one way to do things)
 * 
 * random: K&R malloc is a good malloc
 * 
 */ 