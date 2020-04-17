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
// Don't use tst 
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
	
	//printf("Generated instr: %x\n", inst);

	return inst;
}

static inline uint32_t arm_b_bl(uint32_t L, uint32_t src_addr, uint32_t target_addr) {
    uint32_t u = (arm_AL << 28) | (L << 24) | (0b101 << 25);
	return u;
}

// <add> of an immediate
static inline uint32_t arm_add_imm8(uint8_t rd, uint8_t rs1, uint8_t imm) {
    unimplemented();
}

static inline uint32_t arm_bx(uint8_t reg) {
	uint32_t inst = 0;
	inst = (0b1110) << 28;
	inst |= (0b00010010)<<20;
	inst |= (0xfff) << 8;
	inst |= (0b0001)<<4;
	inst |= (reg & 0b1111);
	return inst;
}

static inline uint32_t arm_b(uint32_t pc, uint32_t imm24) {
	uint32_t inst = 0;
	uint32_t value = imm24 & ~(0x20000000);
	//printf("value is: %x\n", value);
	value |= (0xFFFFFFFF - 0x2000000);
	//printf("value is: %x\n", value);
	value = (value << 2) + 2;
	//printf("value is: %x\n", value);
		//value += pc; 
		//printf("value is: %x\n", value);
	inst = (0b1110) << 28;
	inst |= (0b101) << 25;
	inst |= value & 0xFFFFFF;
	return inst;
}

static inline uint32_t arm_b_srcdest(int32_t src, int32_t dest) {
	uint32_t inst = 0;
	int32_t immed = (~0u >> 8) & ((dest - src - 8));
	uint32_t uimmed = (uint32_t)immed >> 2;
	/*
	uint32_t value = dist & ~(0x20000000);
	//printf("value is: %x\n", value);
	value |= (0xFFFFFFFF - 0x2000000);
	//printf("value is: %x\n", value);
	value = (value << 2) + dist;
	//printf("value is: %x\n", value);
		//value += pc; 
		//printf("value is: %x\n", value);
	inst = (0b1110) << 28;
	inst |= (0b101) << 25;
	inst |= value & 0xFFFFFF;
	*/
	
	/*
	int32_t signed_dist = dist & (0x3FFFFFFF);
	signed_dist = signed_dist >> 2;
	inst = (0b1110) << 28;
	inst |= (0b101) << 25;
	inst |= (signed_dist - 2) & 0xFFFFFF;
	*/

	inst = (0b1110) << 28;
	inst |= (0b101) << 25;
	inst |= uimmed & 0xFFFFFF;
	return inst;
}

static inline uint32_t arm_bl(uint32_t pc, uint32_t imm24) {
	uint32_t inst = 0;
	uint32_t value = imm24 & ~(0x20000000);
	//printf("value is: %x\n", value);
	value |= (0xFFFFFFFF - 0x2000000);
	//printf("value is: %x\n", value);
	value = (value << 2) + 2;
	//printf("value is: %x\n", value);
		//value += pc; 
		//printf("value is: %x\n", value);
	inst = (0b1110) << 28;
	inst |= (0b101) << 25;
	inst |= 1 << 24;
	inst |= value & 0xFFFFFF;
	return inst;
}

static inline uint32_t arm_b3(uint32_t L, int32_t src, int32_t dest) {
	uint32_t inst = 0;
	//uint32_t immed = (~0u >> 8) & ((dest - src - 8)>>2);
	/*
	int32_t immed = (~0u >> 8) & ((dest - src - 8));
	uint32_t uimmed = (uint32_t)immed >> 2;
	*/
	//uint32_t value = dist & ~(0x20000000);
	//printf("value is: %x\n", value);
	//value |= (0xFFFFFFFF - 0x2000000);
	//printf("value is: %x\n", value);
	//value = (value << 2) + 2;
	//printf("value is: %x\n", value);
		//value += pc; 
		//printf("value is: %x\n", value);
	
	/*
	int32_t signed_dist = dist & (0x3FFFFFFF);
	signed_dist = signed_dist >> 2;
	inst = (0b1110) << 28;
	inst |= (0b101) << 25;
	inst |= 1 << 24;
	inst |= (signed_dist - 2) & 0xFFFFFF;
	*/

	/*
	inst = (0b1110) << 28;
	inst |= (0b101) << 25;
	inst |= 1 << 24;
	inst |= uimmed & 0xFFFFFF;
	*/
	int pc = src + 8;
	int offset = dest - pc;
	unsigned imm24 = offset; 
	imm24 = imm24 >> 2;
	imm24 &= (~0U >> 8);
	uint32_t u = (arm_AL << 28) | (L << 24) | (0b101 << 25);
	return u | imm24;
}

static inline uint32_t arm_bl3(uint32_t L, int32_t src, int32_t dest) {
	return arm_b3(1, src, dest);
}

static inline uint32_t arm_blx(uint32_t imm24) {
	uint32_t inst = 0;
	uint32_t value = imm24 & ~(0x20000000);
	//printf("value is: %x\n", value);
	value |= (0xFFFFFFFF - 0x2000000);
	//printf("value is: %x\n", value);
	value = (value << 2) + 2;
	//printf("value is: %x\n", value);
		//value += pc; 
		//printf("value is: %x\n", value);
	inst = (0b1111) << 28;
	inst |= (0b101) << 25;
	inst |= value & 0xFFFFFF;
	return inst;
}

// From page A7-50 of armv6 manual
static inline uint32_t arm_ldr2(uint8_t rd, uint8_t rn, uint8_t rm) {
	uint32_t inst = 0;
	inst |= (0b111001111001) << 20;
	inst |= rn << 16;
	inst |= rd << 12;
	inst |= rm;
	return inst;
}

static inline uint32_t arm_ldr_word_single(uint8_t rd, uint8_t rn) {
	
	uint32_t inst = 0;
	inst |= (0b1110) << 28;
	inst |= (0b0101) << 24;
	inst |= (0b1001) << 20;

	inst |= (rn & 0b1111) << 16;
	inst |= (rd & 0b1111) << 12;

	return inst;
}

static inline uint32_t arm_ldr_word_imm(uint8_t rd, uint8_t rn, uint32_t offset_12) {
	
	uint32_t inst = 0;
	inst |= (0b1110) << 28;
	inst |= (0b0101) << 24;
	inst |= (0b1001) << 20;

	inst |= (rn & 0b1111) << 16;
	inst |= (rd & 0b1111) << 12;
	inst |= offset_12;

	return inst;
}

static inline uint32_t arm_ldr_word_reg(uint8_t rd, uint8_t rn, uint8_t rm) {
	
	uint32_t inst = 0;
	
	assert(rd < 16);
	assert(rn < 16);
	assert(rm < 16);

	inst |= (0b1110) << 28;
	inst |= (0b0111) << 24;
	inst |= (0b1001) << 20;

	inst |= (rn & 0b1111) << 16;
	inst |= (rd & 0b1111) << 12;
	inst |= rm;

	return inst;
}

static inline uint32_t arm_str_word_imm(uint8_t rd, uint8_t rn, uint32_t offset_12) {
	
	uint32_t inst = 0;
	inst |= (0b1110) << 28;
	inst |= (0b0101) << 24;
	inst |= (0b1000) << 20;

	inst |= (rn & 0b1111) << 16;
	inst |= (rd & 0b1111) << 12;
	inst |= offset_12;

	return inst;
}

static inline uint32_t arm_str_word_reg(uint8_t rd, uint8_t rn, uint8_t rm) {
	
	uint32_t inst = 0;
	inst |= (0b1110) << 28;
	inst |= (0b0111) << 24;
	inst |= (0b1000) << 20;

	inst |= (rn & 0b1111) << 16;
	inst |= (rd & 0b1111) << 12;
	inst |= rm;

	return inst;
}

static inline uint32_t arm_str_word_single(uint8_t rd, uint8_t rn) {
	
	uint32_t inst = 0;
	inst |= (0b1110) << 28;
	inst |= (0b0101) << 24;
	inst |= (0b1000) << 20;

	inst |= (rn & 0b1111) << 16;
	inst |= (rd & 0b1111) << 12;

	return inst;
}

static inline uint32_t arm_push(uint8_t reg) {
	
	uint32_t inst = 0;
	inst |= (0b11100101001011010) << 15;

	//if(reg == arm_lr) {
	//	inst |= 1 << 8;
	//}
	
	inst |= (reg) << 12;

	inst |= 4;

	return inst;
}

static inline uint32_t arm_pop(uint8_t reg) {
	
	uint32_t inst = 0;
	inst |= (0b11100100100111010) << 15;

	//if(reg == arm_lr) {
	//	inst |= 1 << 8;
	//}
	
	inst |= (reg) << 12;

	inst |= 4;

	return inst;
}

// load an or immediate and rotate it.
static inline uint32_t 
arm_or_imm_rot(uint8_t rd, uint8_t rs1, uint8_t imm8, uint8_t rot_nbits) {
    unimplemented();
}

#endif
