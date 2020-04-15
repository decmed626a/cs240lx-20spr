#ifndef __ARMV6_ENCODINGS_H__
#define __ARMV6_ENCODINGS_H__
// engler, cs240lx: simplistic instruction encodings for r/pi ARMV6.
// this will compile both on our bare-metal r/pi and unix.

// bit better type checking to use enums.
enum {
    arm_r0 = 0, 
    arm_r1, 
    arm_r2,
    arm_r3,
    arm_r4,
    arm_r5,
    arm_r6,
    arm_r7,
    arm_r8,
    arm_r9,
    arm_r10,
    arm_r11,
    arm_r12,
    arm_r13,
    arm_r14,
    arm_r15,
    arm_sp = arm_r13,
    arm_lr = arm_r14,
    arm_pc = arm_r15
};
_Static_assert(arm_r15 == 15, "bad enum");


// condition code.
enum {
    arm_EQ = 0,
    arm_NE,
    arm_CS,
    arm_CC,
    arm_MI,
    arm_PL,
    arm_VS,
    arm_VC,
    arm_HI,
    arm_LS,
    arm_GE,
    arm_LT,
    arm_GT,
    arm_LE,
    arm_AL,
};
_Static_assert(arm_AL == 0b1110, "bad enum list");

// data processing ops.
enum {
    arm_and_op = 0, 
    arm_eor_op,
    arm_sub_op,
    arm_rsb_op,
    arm_add_op,
    arm_adc_op,
    arm_sbc_op,
    arm_rsc_op,
    arm_tst_op,
    arm_teq_op,
    arm_cmp_op,
    arm_cmn_op,
    arm_orr_op,
    arm_mov_op,
    arm_bic_op,
    arm_mvn_op,
};
_Static_assert(arm_mvn_op == 0b1111, "bad num list");

/************************************************************
 * instruction encodings.  should do:
 *      bx lr
 *      ld *
 *      st *
 *      cmp
 *      branch
 *      alu 
 *      alu_imm
 *      jump
 *      call
 */

// add instruction:
//      add rdst, rs1, rs2
//  - general add instruction: page A4-6 [armv6.pdf]
//  - shift operatnd: page A5-8 [armv6.pdf]
//
// we do not do any carries, so S = 0.
static inline unsigned arm_add(uint8_t rd, uint8_t rs1, uint8_t rs2) {
	unsigned inst = 0;
	
	// Put in condition code
	inst = (0b1110) << 28;
	// Put in bit I
	//inst |= 1 << 25;
	// Get bits 21 to 24
	inst |= (0b0100) << 21;
	// Put in bit S
	//inst |= 1 << 20;
	// Put in Rn
	inst |= rs1 << 16;
	// Put in Rd
	inst |= rd << 12;

	// Do shifting operand
	unsigned shifter = 0;
	// Put in condition code 
	shifter = arm_EQ << 28;
	// Put in opcode
	shifter |= arm_add_op << 21;
	// Put in bit S
	//shifter |= 1 << 20;
	// Put in Rn
	shifter |= rs1 << 16;
	// Put in Rd
	shifter |= rd << 12;
	// Put in Rm
	shifter |= rs2;

	inst |= (shifter & 0x7FF);
	
	printf("Generated instr: %x\n", inst);

	return inst;

	/*
	unsigned inst = 0;
	inst = 0xffff0fff | (rd << 13) | (rs1 << 13) | (rs2 << 13);

	printf("Generated instr: %x\n", inst);

	return inst;
	*/
}

// <add> of an immediate
static inline uint32_t arm_add_imm8(uint8_t rd, uint8_t rs1, uint8_t imm) {
    unimplemented();
}

static inline uint32_t arm_bx(uint8_t reg) {
    unimplemented();
}

// load an or immediate and rotate it.
static inline uint32_t 
arm_or_imm_rot(uint8_t rd, uint8_t rs1, uint8_t imm8, uint8_t rot_nbits) {
    unimplemented();
}

#endif
