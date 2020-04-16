#include <assert.h>
#include <sys/types.h>
#include <string.h>
#include "libunix.h"
#include <unistd.h>
#include "code-gen.h"
#include "armv6-insts.h"
#include "arm_gen.h"

/*
 *  1. emits <insts> into a temporary file.
 *  2. compiles it.
 *  3. reads back in.
 *  4. returns pointer to it.
 */
uint32_t *insts_emit(unsigned *nbytes, char *insts) {
    // check libunix.h --- create_file, write_exact, run_system, read_file.
	const char asm_filename[10] = "tempasm.s";
	const char machine_filename[8] = "tempasm";
	int asm_fd = create_file(asm_filename);
	write_exact(asm_fd, (void*) insts, strlen(insts));
	write_exact(asm_fd, "\n", 1);
	run_system("arm-none-eabi-as --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6zk tempasm.s -o tempasm.o");
	run_system("arm-none-eabi-objcopy tempasm.o -O binary tempasm.bin");
	void* code_ptr = read_file(nbytes, "tempasm.bin");
	close(asm_fd);
	return (uint32_t*)code_ptr;
}


/*
 * a cross-checking hack that uses the native GNU compiler/assembler to 
 * check our instruction encodings.
 *  1. compiles <insts>
 *  2. compares <code,nbytes> to it for equivalance.
 *  3. prints out a useful error message if it did not succeed!!
 */
void insts_check(char *insts, uint32_t *code, unsigned nbytes) {
    // make sure you print out something useful on mismatch!
	uint32_t* buf = (uint32_t*)malloc(nbytes);
	buf = insts_emit(&nbytes, insts);
	if (0 == memcmp(buf, code, nbytes)) {
		//printf("Success\n");
		
		printf("success: correctly encoded < %s > as [ 0x%x ]\n", insts, *code);
		
	} else {
		printf("failure: < %s > should be [ 0x%x ], not [ 0x%x ]\n", insts, *buf, *code);
	}
}

// check a single instruction.
void check_one_inst(char *insts, uint32_t inst) {
    return insts_check(insts, &inst, 4);
}

// helper function to make reverse engineering instructions a bit easier.
void insts_print(char *insts) {
    // emit <insts>
    unsigned gen_nbytes;
    uint32_t *gen = insts_emit(&gen_nbytes, insts);

    // print the result.
    output("getting encoding for: < %20s >\t= [", insts);
    unsigned n = gen_nbytes / 4;
    for(int i = 0; i < n; i++)
         output(" 0x%x ", gen[i]);
    output("]\n");
}


// helper function for reverse engineering.  you should refactor its interface
// so your code is better.
uint32_t emit_rrr(const char* op, const char *d, const char *s1, const char *s2) {
    char buf[1024];
    sprintf(buf, "%s %s, %s, %s", op, d, s1, s2);

    uint32_t n;
    uint32_t *c = insts_emit(&n, buf);
    assert(n == 4);
    return *c;
}

// helper function for reverse engineering.  you should refactor its interface
// so your code is better.
uint32_t emit_2rr(const char* op, const char *d, const char *s1) {
    char buf[1024];
    sprintf(buf, "%s %s, %s", op, d, s1);

    uint32_t n;
    uint32_t *c = insts_emit(&n, buf);
    assert(n == 4);
    return *c;
}

unsigned solve(int run_number, unsigned* op, const char* opcode, const char** dst, const char** src1, const char** src2) {
	
	unsigned offset = 0;

	const char* temp;
	if(0 == run_number) {
		temp = *dst;
	} else if (1 == run_number) {
		temp = *src1;
	} else {
		temp = *src2;
	}

    uint32_t always_0 = ~0, always_1 = ~0;

	// DESTINATION OFFSET
    // compute any bits that changed as we vary d.
    for(unsigned i = 0; temp[i]; i++) {
		uint32_t u = 0;
		if(0 == run_number) {
        	u = emit_rrr(opcode, dst[i], *src1, *src2);
		} else if (1 == run_number) {
        	u = emit_rrr(opcode, *dst, src1[i], *src2);
		} else {
        	u = emit_rrr(opcode, *dst, *src1, src2[i]);
		}
        // if a bit is always 0 then it will be 1 in always_0
        always_0 &= ~u;

        // if a bit is always 1 it will be 1 in always_1, otherwise 0
        always_1 &= u;
    }

    if(always_0 & always_1) 
        panic("impossible overlap: always_0 = %x, always_1 %x\n", 
            always_0, always_1);

    // bits that never changed
    uint32_t never_changed = always_0 | always_1;
    // bits that changed: these are the register bits.
    uint32_t changed = ~never_changed;

    //output("register dst are bits set in: %x\n", changed);

    // find the offset.  we assume register bits are contig and within 0xf
    offset = ffs(changed) - 1;
	//printf("d_off: %d", d_off);
    
	// check that bits are contig and at most 4 bits are set.
    if(((changed >> offset) & ~0xf) != 0)
        panic("weird instruction!  expecting at most 4 contig bits: %x\n", changed);
    
	// refine the opcode.
	if(0 == run_number) {
    	*op &= never_changed;

		// Mask opcode with dummy call to emit_rrr
		*op &= emit_rrr(opcode, *dst, *src1, *src2);

	}
	return offset;

}

unsigned solve_2rr(int run_number, unsigned* op, const char* opcode, const char** dst, const char** src1) {
	
	unsigned offset = 0;

	const char* temp;
	if(0 == run_number) {
		temp = *dst;
	} else {
		temp = *src1;
	}

    uint32_t always_0 = ~0, always_1 = ~0;

	// DESTINATION OFFSET
    // compute any bits that changed as we vary d.
    for(unsigned i = 0; temp[i]; i++) {
		uint32_t u = 0;
		if(0 == run_number) {
        	u = emit_2rr(opcode, dst[i], *src1);
		} else {
        	u = emit_2rr(opcode, *dst, src1[i]);
		}
        // if a bit is always 0 then it will be 1 in always_0
        always_0 &= ~u;

        // if a bit is always 1 it will be 1 in always_1, otherwise 0
        always_1 &= u;
    }

    if(always_0 & always_1) 
        panic("impossible overlap: always_0 = %x, always_1 %x\n", 
            always_0, always_1);

    // bits that never changed
    uint32_t never_changed = always_0 | always_1;
    // bits that changed: these are the register bits.
    uint32_t changed = ~never_changed;

    //output("register dst are bits set in: %x\n", changed);

    // find the offset.  we assume register bits are contig and within 0xf
    offset = ffs(changed) - 1;
	//printf("d_off: %d", d_off);
    
	// check that bits are contig and at most 4 bits are set.
    // TODO: IS THIS NEEDED for cmn, orr, and bic?
	//if(((changed >> offset) & ~0xf) != 0)
    //    panic("weird instruction!  expecting at most 4 contig bits: %x\n", changed);
    
	// refine the opcode.
	if(0 == run_number) {
    	*op &= never_changed;

		// Mask opcode with dummy call to emit_rrr
		*op &= emit_2rr(opcode, *dst, *src1);

	}
	return offset;
}

// overly-specific.  some assumptions:
//  1. each register is encoded with its number (r0 = 0, r1 = 1)
//  2. related: all register fields are contiguous.
//
// NOTE: you should probably change this so you can emit all instructions 
// all at once, read in, and then solve all at once.
//
// For lab:
//  1. complete this code so that it solves for the other registers.
//  2. refactor so that you can reused the solving code vs cut and pasting it.
//  3. extend system_* so that it returns an error.
//  4. emit code to check that the derived encoding is correct.
//  5. emit if statements to checks for illegal registers (those not in <src1>,
//    <src2>, <dst>).
void derive_op_rrr(const char *name, const char *opcode, 
        const char **dst, const char **src1, const char **src2) {

	int run_number = 0;
    const char *s1 = src1[0];
    const char *s2 = src2[0];
    const char *d = dst[0];
	unsigned op = ~0;
    assert(d && s1 && s2);

	unsigned d_off = solve(run_number, &op, opcode, dst, &s1, &s2);
	++run_number;
	unsigned src1_off = solve(run_number, &op, opcode, &d, src1, &s2);
	++run_number;
	unsigned src2_off = solve(run_number, &op, opcode, &d, &s1, src2);

    // emit: NOTE: obviously, currently <src1_off>, <src2_off> are not 
    // defined (so solve for them) and opcode needs to be refined more.
    output("static int %s(uint32_t dst, uint32_t src1, uint32_t src2) {\n", name);
    output("    return 0x%x | (dst << %d) | (src1 << %d) | (src2 << %d);\n",
                op,
                d_off,
                src1_off,
                src2_off);
    output("}\n");
}

void derive_op_2rr(const char *name, const char *opcode, 
        const char **dst, const char **src1) {

	int run_number = 0;
    const char *s1 = src1[0];
    const char *d = dst[0];
	unsigned op = ~0;
    assert(d && s1);

	unsigned d_off = solve_2rr(run_number, &op, opcode, dst, &s1);
	++run_number;
	unsigned src1_off = solve_2rr(run_number, &op, opcode, &d, src1);

    // emit: NOTE: obviously, currently <src1_off>, <src2_off> are not 
    // defined (so solve for them) and opcode needs to be refined more.
    output("static int %s(uint32_t dst, uint32_t src1) {\n", name);
    output("    return 0x%x | (dst << %d) | (src1 << %d);\n",
                op,
                d_off,
                src1_off);
    output("}\n");
}

/*
 * 1. we start by using the compiler / assembler tool chain to get / check
 *    instruction encodings.  this is sleazy and low-rent.   however, it 
 *    lets us get quick and dirty results, removing a bunch of the mystery.
 *
 * 2. after doing so we encode things "the right way" by using the armv6
 *    manual (esp chapters a3,a4,a5).  this lets you see how things are 
 *    put together.  but it is tedious.
 *
 * 3. to side-step tedium we use a variant of (1) to reverse engineer 
 *    the result.
 *
 *    we are only doing a small number of instructions today to get checked off
 *    (you, of course, are more than welcome to do a very thorough set) and focus
 *    on getting each method running from beginning to end.
 *
 * 4. then extend to a more thorough set of instructions: branches, loading
 *    a 32-bit constant, function calls.
 *
 * 5. use (4) to make a simple object oriented interface setup.
 *    you'll need: 
 *      - loads of 32-bit immediates
 *      - able to push onto a stack.
 *      - able to do a non-linking function call.
 */
int main(void) {
#if 0
    // part 1: implement the code to do this.
    output("-----------------------------------------\n");
    output("part1: checking: correctly generating assembly.\n");
    insts_print("add r0, r0, r1");
    insts_print("bx lr");
    insts_print("mov r0, #1");
    insts_print("nop");
    output("\n");
    output("success!\n");

    // part 2: implement the code so these checks pass.
    // these should all pass.
    output("\n-----------------------------------------\n");
    output("part 2: checking we correctly compare asm to machine code.\n");
    check_one_inst("add r0, r0, r1", 0xe0800001);
    check_one_inst("bx lr", 0xe12fff1e);
    check_one_inst("mov r0, #1", 0xe3a00001);
    check_one_inst("nop", 0xe320f000);
    output("success!\n");

    // part 3: check that you can correctly encode an add instruction.
    output("\n-----------------------------------------\n");
    output("part3: checking that we can generate an <add> by hand\n");
    check_one_inst("add r0, r1, r2", arm_add(arm_r0, arm_r1, arm_r2));
    check_one_inst("add r3, r4, r5", arm_add(arm_r3, arm_r4, arm_r5));
    check_one_inst("add r6, r7, r8", arm_add(arm_r6, arm_r7, arm_r8));
    check_one_inst("add r9, r10, r11", arm_add(arm_r9, arm_r10, arm_r11));
    check_one_inst("add r12, r13, r14", arm_add(arm_r12, arm_r13, arm_r14));
    check_one_inst("add r15, r7, r3", arm_add(arm_r15, arm_r7, arm_r3));
    output("success!\n");

    // part 4: implement the code so it will derive the add instruction.
    output("\n-----------------------------------------\n");
    output("part4: checking that we can reverse engineer an <add>\n");

    const char *all_regs[] = {
                "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
                0 
    };
    // XXX: should probably pass a bitmask in instead.
    derive_op_rrr("arm_and", "and", all_regs,all_regs,all_regs);
    derive_op_rrr("arm_eor", "eor", all_regs,all_regs,all_regs);
    derive_op_rrr("arm_sub", "sub", all_regs,all_regs,all_regs);
    derive_op_rrr("arm_rsb", "rsb", all_regs,all_regs,all_regs);
    derive_op_rrr("arm_add", "add", all_regs,all_regs,all_regs);
    derive_op_rrr("arm_adc", "adc", all_regs,all_regs,all_regs);
    derive_op_rrr("arm_sbc", "sbc", all_regs,all_regs,all_regs);
    derive_op_rrr("arm_rsc", "rsc", all_regs,all_regs,all_regs);
    derive_op_2rr("arm_tst", "tst", all_regs,all_regs);
    derive_op_2rr("arm_teq", "teq", all_regs,all_regs);
    derive_op_2rr("arm_cmp", "cmp", all_regs,all_regs);
    // cmn triggers contiguous 4 bit warning
	derive_op_2rr("arm_cmn", "cmn", all_regs,all_regs);
    // orr triggers contiguous 4 bit warning
    derive_op_2rr("arm_orr", "orr", all_regs,all_regs);
    derive_op_2rr("arm_mov", "mov", all_regs,all_regs);
    // bic triggers contiguous 4 bit warning
    derive_op_2rr("arm_bic", "bic", all_regs,all_regs);
    derive_op_2rr("arm_mvn", "mvn", all_regs,all_regs);
    output("did something: now use the generated code in the checks above!\n");

	// check generated instructions
    check_one_inst("and r0, r1, r2", arm_and(arm_r0, arm_r1, arm_r2));
    check_one_inst("eor r0, r1, r2", arm_eor(arm_r0, arm_r1, arm_r2));
    check_one_inst("sub r0, r1, r2", arm_sub(arm_r0, arm_r1, arm_r2));
    check_one_inst("rsb r0, r1, r2", arm_rsb(arm_r0, arm_r1, arm_r2));
    check_one_inst("rsb r0, r1, r2", arm_rsb(arm_r0, arm_r1, arm_r2));
    check_one_inst("add r0, r1, r2", derive_arm_add(arm_r0, arm_r1, arm_r2));
    check_one_inst("adc r0, r1, r2", arm_adc(arm_r0, arm_r1, arm_r2));
    check_one_inst("sbc r0, r1, r2", arm_sbc(arm_r0, arm_r1, arm_r2));
    check_one_inst("rsc r0, r1, r2", arm_rsc(arm_r0, arm_r1, arm_r2));
    check_one_inst("tst r0, r1", arm_tst(arm_r0, arm_r1));
    check_one_inst("teq r0, r1", arm_teq(arm_r0, arm_r1));
    check_one_inst("cmp r0, r1", arm_cmp(arm_r0, arm_r1));
    check_one_inst("cmn r0, r1", arm_cmn(arm_r0, arm_r1));
    check_one_inst("orr r0, r1", arm_orr(arm_r0, arm_r1));
    check_one_inst("mov r0, r1", arm_mov(arm_r0, arm_r1));
    check_one_inst("bic r0, r1", arm_bic(arm_r0, arm_r1));
    check_one_inst("mvn r0, r1", arm_mvn(arm_r0, arm_r1));
	// get encodings for other instructions, loads, stores, branches, etc.
  #endif 
	output("\n-----------------------------------------\n");
    output("part5: checking that we can generate a <bx lr> by hand\n");
    check_one_inst("bx lr", arm_bx(14));
    check_one_inst("b 0x12bb", arm_b(15, 0x12bb));
    check_one_inst("b 0x0", arm_b(15, 0x0));
    check_one_inst("b 0xdeadbeef", arm_b(15, 0xdeadbeef));
    check_one_inst("b 0xc001d00d", arm_b(15, 0xc001d00d));
    check_one_inst("b 0xffffffff", arm_b(15, 0xffffffff));
    check_one_inst("bl 0x12bb", arm_bl(15, 0x12bb));
    check_one_inst("bl 0x0", arm_bl(15, 0x0));
    check_one_inst("bl 0xdeadbeef", arm_bl(15, 0xdeadbeef));
    check_one_inst("bl 0xc001d00d", arm_bl(15, 0xc001d00d));
    check_one_inst("bl 0xffffffff", arm_bl(15, 0xffffffff));
    check_one_inst("blx 0x12bb", arm_blx(0x12bb));
    check_one_inst("blx 0x0", arm_blx(0x0));
    check_one_inst("blx 0xdeadbeef", arm_blx(0xdeadbeef));
    check_one_inst("blx 0xc001d00d", arm_blx(0xc001d00d));
    check_one_inst("blx 0xffffffff", arm_blx(0xffffffff));
    check_one_inst("ldr r1, [r2]", arm_ldr_word_single(1, 2));
    check_one_inst("str r1, [r2]", arm_str_word_single(1, 2));
    check_one_inst("ldr r1, [r2, #50]", arm_ldr_word_imm(1, 2, 50));
    check_one_inst("str r1, [r2, #50]", arm_str_word_imm(1, 2, 50));
    check_one_inst("ldr r1, [r2, r3]", arm_ldr_word_reg(1, 2, 3));
    check_one_inst("str r1, [r2, r3]", arm_str_word_reg(1, 2, 3));
    
	return 0;
}
