#include "stdint.h"

static int arm_and(uint32_t dst, uint32_t src1, uint32_t src2) {
    return 0xe0000000 | (dst << 12) | (src1 << 16) | (src2 << 0);
}
static int arm_eor(uint32_t dst, uint32_t src1, uint32_t src2) {
    return 0xe0200000 | (dst << 12) | (src1 << 16) | (src2 << 0);
}
static int arm_sub(uint32_t dst, uint32_t src1, uint32_t src2) {
    return 0xe0400000 | (dst << 12) | (src1 << 16) | (src2 << 0);
}
static int arm_rsb(uint32_t dst, uint32_t src1, uint32_t src2) {
    return 0xe0600000 | (dst << 12) | (src1 << 16) | (src2 << 0);
}
static int derive_arm_add(uint32_t dst, uint32_t src1, uint32_t src2) {
    return 0xe0800000 | (dst << 12) | (src1 << 16) | (src2 << 0);
}
static int arm_adc(uint32_t dst, uint32_t src1, uint32_t src2) {
    return 0xe0a00000 | (dst << 12) | (src1 << 16) | (src2 << 0);
}
static int arm_sbc(uint32_t dst, uint32_t src1, uint32_t src2) {
    return 0xe0c00000 | (dst << 12) | (src1 << 16) | (src2 << 0);
}
static int arm_rsc(uint32_t dst, uint32_t src1, uint32_t src2) {
    return 0xe0e00000 | (dst << 12) | (src1 << 16) | (src2 << 0);
}
static int arm_tst(uint32_t dst, uint32_t src1) {
    return 0xe1100000 | (dst << 16) | (src1 << 0);
}
static int arm_teq(uint32_t dst, uint32_t src1) {
    return 0xe1300000 | (dst << 16) | (src1 << 0);
}
static int arm_cmp(uint32_t dst, uint32_t src1) {
    return 0xe1500000 | (dst << 16) | (src1 << 0);
}
static int arm_cmn(uint32_t dst, uint32_t src1) {
    return 0xe1700000 | (dst << 16) | (src1 << 0);
}
static int arm_orr(uint32_t dst, uint32_t src1) {
    return 0xe1800000 | (dst << 12) | (src1 << 0);
}
static int arm_mov(uint32_t dst, uint32_t src1) {
    return 0xe1a00000 | (dst << 12) | (src1 << 0);
}
static int arm_bic(uint32_t dst, uint32_t src1) {
    return 0xe1c00000 | (dst << 12) | (src1 << 0);
}
static int arm_mvn(uint32_t dst, uint32_t src1) {
    return 0xe1e00000 | (dst << 12) | (src1 << 0);
}
