/*************************************************************************
 * engler, cs240lx: Purify/Boehm style leak checker/gc starter code.
 *
 * We've made a bunch of simplifications.  Given lab constraints:
 * people talk about trading space for time or vs. but the real trick 
 * is how to trade space and time for less IQ needed to see something 
 * is correct. (I haven't really done a good job of that here, but
 * it's a goal.)
 *
 */
#include "rpi.h"
#include "rpi-constants.h"
#include "ckalloc-internal.h"
#include "libc/helper-macros.h"

/****************************************************************
 * code you implement is below.
 */

static struct heap_info info;

// quick check that the pointer is between the start of
// the heap and the last allocated heap pointer.  saves us 
// walk through all heap blocks.
//
// we could warn if the pointer is within some amount of slop
// so that you can detect some simple overruns?
static int in_heap(void *p) {
    return ((p >= info.heap_start) && (p <= info.heap_end));
}

// 1 if addr is in h's range  0 otherwise
static int in_range(hdr_t * h, uint32_t addr){
    uint32_t start = (uint32_t) b_alloc_ptr(h);
    uint32_t end = start + h->nbytes_alloc;
    // output("checking in range %p\n", addr);
    if(addr >= start && addr < end){
        return 1;
    }
    return 0;
}

// given potential address <addr>, returns:
//  - 0 if <addr> does not correspond to an address range of 
//    any allocated block.
//  - the associated header otherwise (even if freed: caller should
//    check and decide what to do in that case).
//
// XXX: you'd want to abstract this some so that you can use it with
// other allocators.  our leak/gc isn't really allocator specific.
static hdr_t *is_ptr(uint32_t addr) {
    void *p = (void*)addr;
    
    if(!in_heap(p))
        return 0;

	hdr_t* curr_hdr = ck_first_hdr();
	while(curr_hdr) {
		
		if(addr == (uint32_t)b_alloc_ptr(curr_hdr)) {
			curr_hdr->refs_start++;
    		//curr_hdr->cksum = hdr_cksum(curr_hdr);
			return curr_hdr;
		}
		
		if(addr > (uint32_t)b_alloc_ptr(curr_hdr) && 
		   addr <= (uint32_t)b_alloc_ptr(curr_hdr) + curr_hdr->nbytes_alloc) {
			curr_hdr->refs_middle++; 
    		//curr_hdr->cksum = hdr_cksum(curr_hdr);
			return curr_hdr;
		}
		curr_hdr = ck_next_hdr(curr_hdr);
		
	}
    // output("could not find pointer %p\n", addr);
    return 0;
}

// mark phase:
//  - iterate over the words in the range [p,e], marking any block 
//    potentially referenced.
//  - if we mark a block for the first time, recurse over its memory
//    as well.
//
// NOTE: if we have lots of words, could be faster with shadow memory / a lookup
// table.  however, given our small sizes, this stupid search may well be faster :)
// If you switch: measure speedup!
//
#include "libc/helper-macros.h"
static void mark(uint32_t *p, uint32_t *e) {
    assert(p<e);
    // maybe keep this same thing?
    assert(aligned(p,4));
    assert(aligned(e,4));
    // output("marking %p to %p\n",p, e);
    uint32_t end = (uint32_t) e;
    uint32_t * curr = p;
    hdr_t * curr_hdr;
    while(curr != e){
        if((curr_hdr = is_ptr(*curr)) != 0){
            // is a pointer
            if(*curr == (uint32_t) b_alloc_ptr(curr_hdr)){
                // output("marked start %p\n", curr_hdr);
                curr_hdr->refs_start++;
            }else{
                // output("marked middle %p\n", curr_hdr);
                curr_hdr->refs_middle++;
            }
            if(!curr_hdr->mark){

                // output("found %p\n",b_alloc_ptr(curr_hdr) );
                curr_hdr->mark = 1;
                // output("marked %p\n",b_alloc_ptr(curr_hdr) );
                // recurse
                mark((uint32_t *) b_alloc_ptr(curr_hdr), (uint32_t *) ((char *)b_alloc_ptr(curr_hdr) + curr_hdr->nbytes_alloc));
            }
        }
        curr ++;
    }
}


// do a sweep, warning about any leaks.
//
//
static unsigned sweep_leak(int warn_no_start_ref_p) {
	unsigned nblocks = 0, errors = 0, maybe_errors=0;
	output("---------------------------------------------------------\n");
	output("checking for leaks:\n");

	hdr_t* curr_hdr = ck_first_hdr();
	while(curr_hdr != 0) {
		if(curr_hdr->state == ALLOCED) {		
			if(curr_hdr->mark == 0) {
				trace("ERROR:GC:DEFINITE LEAK of %x\n", b_alloc_ptr(curr_hdr));
				errors++; //= curr_hdr->refs_start;
			} else {
				curr_hdr->mark = 0;
                if(warn_no_start_ref_p && curr_hdr->refs_start == 0){
					trace("ERROR:GC:MAYBE LEAK of %x (no pointer to the start)\n",
						  b_alloc_ptr(curr_hdr));
					maybe_errors++; //= curr_hdr->refs_middle;
				}
			}
		}
		nblocks++;
		curr_hdr = ck_next_hdr(curr_hdr);
	}

	trace("\tGC:Checked %d blocks.\n", nblocks);
	if(!errors && !maybe_errors)
		trace("\t\tGC:SUCCESS: No leaks found!\n");
	else
		trace("\t\tGC:ERRORS: %d errors, %d maybe_errors\n", 
						errors, maybe_errors);
	output("----------------------------------------------------------\n");
	return errors + maybe_errors;
}

# if 0
static unsigned sweep_leak(int warn_no_start_ref_p) {
	unsigned nblocks = 0, errors = 0, maybe_errors=0;
	output("---------------------------------------------------------\n");
	output("checking for leaks:\n");

    
    for(hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h)) {
        nblocks++;
        if(h->state == ALLOCED){
            // output("%p\n", b_alloc_ptr(h));
            if(h->mark == 0){
                errors++;
            }else{
                h->mark = 0;
                if(warn_no_start_ref_p && h->refs_start == 0){
                    maybe_errors++;
                }
            }
        }else{
            // ckfree(b_alloc_ptr(h));
        }
    }


	trace("\tGC:Checked %d blocks.\n", nblocks);
	if(!errors && !maybe_errors)
		trace("\t\tGC:SUCCESS: No leaks found!\n");
	else
		trace("\t\tGC:ERRORS: %d errors, %d maybe_errors\n", 
						errors, maybe_errors);
	output("----------------------------------------------------------\n");
	return errors + maybe_errors;
}
#endif 

// write the assembly to dump all registers.
// need this to be in a seperate assembly file since gcc 
// seems to be too smart for its own good.
void dump_regs(uint32_t *v, ...);

// a very slow leak checker.
static void mark_all(void) {
    // slow: should not need this: remove after your code
    // works.
    for(hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h)) {
        h->mark = h->refs_start = h->refs_middle = 0;
    	//h->cksum = hdr_cksum(h);
    }

    // get start and end of heap so we can do quick checks
    struct heap_info i = heap_info();
	info = i;

	// additional checks?
	//unimplemented();

	// pointers can be on the stack, in registers, or in the heap itself.

    // get all the registers.
    uint32_t regs[16];
    // should kill caller-saved registers
    dump_regs(regs);
    assert(regs[0] == (uint32_t)&regs[0]);

	for(int i = 0; i < 4; i++) {
		regs[i] = 0;
	}
	// additional checks
    //unimplemented();

    mark(&regs[4], &regs[14]);

    // mark the stack: we are assuming only a single
    // stack.  note: stack grows down.
	// Use the stack pointer
    uint32_t *stack_top = (void*)STACK_ADDR;
	mark((uint32_t*)regs[13], stack_top);

    // these symbols are defined in our memmap
	extern uint32_t __bss_start__, __bss_end__;
	mark(&__bss_start__, &__bss_end__);

	extern uint32_t __data_start__, __data_end__;
	mark(&__data_start__, &__data_end__);
}

/***********************************************************
 * testing code: we give you this.
 */

// return number of bytes allocated?  freed?  leaked?
// how do we check people?
unsigned ck_find_leaks(int warn_no_start_ref_p) {
    mark_all();
    return sweep_leak(warn_no_start_ref_p);
}

// used for tests.  just keep it here.
void check_no_leak(void) {
    // when in the original tests, it seemed gcc was messing 
    // around with these checks since it didn't see that 
    // the pointer could escape.
    gcc_mb();
    if(ck_heap_errors())
        panic("GC: invalid error!!\n");
    else
        trace("GC: SUCCESS heap checked out\n");
    if(ck_find_leaks(1))
        panic("GC: should have no leaks!\n");
    else
        trace("GC: SUCCESS: no leaks!\n");
    gcc_mb();
}

// used for tests.  just keep it here.
unsigned check_should_leak(void) {
    // when in the original tests, it seemed gcc was messing 
    // around with these checks since it didn't see that 
    // the pointer could escape.
    gcc_mb();
    if(ck_heap_errors())
        panic("GC: invalid error!!\n");
    else
        trace("GC: SUCCESS heap checked out\n");

    unsigned nleaks = ck_find_leaks(1);
    if(!nleaks)
        panic("GC: should have leaks!\n");
    else
        trace("GC: SUCCESS: found %d leaks!\n", nleaks);
    gcc_mb();
    return nleaks;
}

void implement_this(void) { 
    panic("did not implement dump_regs!\n");
}

// similar to sweep_leak: but mark unreferenced blocks as FREED.
// similar to sweep_leak: but mark unreferenced blocks as FREED.
static unsigned sweep_free(void) {
	unsigned nblocks = 0, nfreed=0, nbytes_freed = 0;
	output("---------------------------------------------------------\n");
	output("compacting:\n");

	hdr_t* curr_hdr = ck_first_hdr();
	while(curr_hdr != 0) {
		if(curr_hdr->state == ALLOCED) {		
			if(curr_hdr->mark == 0) {
			    nbytes_freed += curr_hdr->nbytes_alloc;
				nfreed++;
				void* ptr = b_alloc_ptr(curr_hdr);
				trace("GC:FREEing ptr=%x\n", ptr);
                ckfree(ptr);
			}
		}
		nblocks++;
		curr_hdr = ck_next_hdr(curr_hdr);
	}
	

    // i used this when freeing.
    // trace("GC:FREEing ptr=%x\n", ptr);
    // ckfree(ptr);

	trace("\tGC:Checked %d blocks, freed %d, %d bytes\n", nblocks, nfreed, nbytes_freed);

    struct heap_info info = heap_info();
	trace("\tGC:total allocated = %d, total deallocated = %d\n", 
                                info.nbytes_alloced, info.nbytes_freed);
	output("----------------------------------------------------------\n");
    return nbytes_freed;
}

unsigned ck_gc(void) {
    mark_all();
    unsigned nbytes = sweep_free();

    // perhaps coalesce these and give back to heap.  will have to modify last.

    return nbytes;
}
