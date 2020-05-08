// engler, cs240lx initial driver code.
//
// everything is put in here so it's easy to find.  when it works,
// seperate it out.
//
// KEY: document why you are doing what you are doing.
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
// 
// also: a sentence or two will go a long way in a year when you want 
// to re-use the code.
#include "rpi.h"
#include "i2c.h"
#include "lsm6ds33.h"
#include <limits.h>

/**********************************************************************
 * some helpers
 */

enum { VAL_WHO_AM_I      = 0x69, };

// read register <reg> from i2c device <addr>
uint8_t imu_rd(uint8_t addr, uint8_t reg) {
    i2c_write(addr, &reg, 1);
        
    uint8_t v;
    i2c_read(addr,  &v, 1);
    return v;
}

// write register <reg> with value <v> 
void imu_wr(uint8_t addr, uint8_t reg, uint8_t v) {
    uint8_t data[2];
    data[0] = reg;
    data[1] = v;
    i2c_write(addr, data, 2);
    // printk("writeReg: %x=%x\n", reg, v);
}

// <base_reg> = lowest reg in sequence. --- hw will auto-increment 
// if you set IF_INC during initialization.
int imu_rd_n(uint8_t addr, uint8_t base_reg, uint8_t *v, uint32_t n) {
        i2c_write(addr, (void*) &base_reg, 1);
        return i2c_read(addr, v, n);
}

/**********************************************************************
 * simple accel setup and use
 */

// returns the raw value from the sensor.
static short mg_raw(uint8_t hi, uint8_t lo) {
	short combo_mg = (hi << 8) | lo;
	return combo_mg;
}
// returns milligauss, integer
static int mg_scaled(int mg_scale, int v) {
	return (v *  mg_scale * 1000) / SHRT_MAX;
}


static void test_mg(int expected, uint8_t h, uint8_t l, unsigned g) {
    int s_i = mg_scaled(mg_raw(h,l),g);
    printk("expect = %d, got %d\n", expected, s_i);
    assert(s_i == expected);
}

static imu_xyz_t xyz_mk(int x, int y, int z) {
    return (imu_xyz_t){.x = x, .y = y, .z = z};
}

// takes in raw data and scales it.
imu_xyz_t accel_scale(accel_t *h, imu_xyz_t xyz) {
    int g = h->g;
    int x = mg_scaled(h->g, xyz.x);
    int y = mg_scaled(h->g, xyz.y);
    int z = mg_scaled(h->g, xyz.z);
    return xyz_mk(x,y,z);
}

int accel_has_data(accel_t *h) {
    // read from bit 1 of STATUS_REG to see if
	// accelerometer is available, p.56 of datasheet
	unsigned stat_val = imu_rd(h->addr, STATUS_REG) & 1;
	if(stat_val != 0) {
		return 1;
	} else {
		return 0;
	}
}

// block until there is data and then return it (raw)
//
// p26 interprets the data.
// if high bit is set, then accel is negative.
//
// read them all at once for consistent
// readings using autoincrement.
// these are blocking.  perhaps make non-block?
// returns raw, unscaled values.
imu_xyz_t accel_rd(accel_t *h) {
    // not sure if we have to drain the queue if there are more readings?

    unsigned mg_scale = h->g;
    uint8_t addr = h->addr;
	while(!accel_has_data(h)) {;}

	uint8_t vals[6];

	imu_rd_n(h->addr, OUTX_L_XL, vals, 6);

	int x = mg_raw(vals[1], vals[0]);
	int y = mg_raw(vals[3], vals[2]);
	int z = mg_raw(vals[5], vals[4]);

	printk("x: %d\n", x);
	printk("y: %d\n", y);
	printk("z: %d\n", z);

	imu_xyz_t accel_data = {.x=x, .y=y, .z=z};
	return accel_data;
}

// first do the cookbook from the data sheet.
// make sure:
//  - you use BDU and auto-increment.
//  - you get reasonable results!
//
// note: the initial readings are garbage!  skip these (see the data sheet)
accel_t accel_init(uint8_t addr, lsm6ds33_g_t g, lsm6ds33_hz_t hz) {
    dev_barrier();

    // some sanity checking
    switch(hz) {
    // these only work for "low-power" i think (p10)
    case lsm6ds33_1660hz:
    case lsm6ds33_3330hz:
    case lsm6ds33_6660hz:
        panic("accel: hz setting of %x does not appear to work\n", hz);
    default:
        break;
    }
    if(!legal_G(g))
        panic("invalid G value: %x\n", g);
    if(hz > lsm6ds33_6660hz)
        panic("invalid hz: %x\n", hz);

    // see header: pull out the scale and the bit pattern.
    unsigned g_scale = g >> 16;
    unsigned g_bits = g&0xff;
    assert(legal_g_bits(g_bits));

	// Enable X, Y, and Z output in register CTRL9_XL
	// Put in p. 53 of the datasheet
	imu_wr(addr, CTRL9_XL, (1 << 3) | (1 << 4) | (1 << 5));

	// Enable BDU in register CTRL3_C (p. 49 of datasheet)
	// Also need IF_INC enabled of same register
	imu_wr(addr, CTRL3_C, (1 << 6) | (1 << 2));

	// Set to high performance mode, 416Hz
	// Set CTRL1_XL (p. 46)
	imu_wr(addr, CTRL1_XL, (1 << 6) | (1 << 5));

	// Populate acclerometer struct
	accel_t accel_struct = {.addr=addr, .g=g_scale, .hz=hz};

	// Delay to get values out
	delay_ms(20);

	dev_barrier();

	return accel_struct;
}


void do_accel_test(void) {
    // initialize accel.
    accel_t h = accel_init(lsm6ds33_default_addr, lsm6ds33_2g, lsm6ds33_416hz);

    int x,y,z;

    // p 26 of application note.
    test_mg(0, 0x00, 0x00, 2);
    test_mg(350, 0x16, 0x69, 2);
    test_mg(1000, 0x40, 0x09, 2);
    test_mg(-350, 0xe9, 0x97, 2);
    test_mg(-1000, 0xbf, 0xf7, 2);

    for(int i = 0; i < 10; i++) {
        imu_xyz_t v = accel_rd(&h);
        printk("accel raw values: x=%d,y=%d,z=%d\n", v.x,v.y,v.z);

        v = accel_scale(&h, v);
        printk("accel scaled values in mg: x=%d,y=%d,z=%d\n", v.x,v.y,v.z);

        delay_ms(500);
    }
}

/**********************************************************************
 * trivial driver.
 */
void notmain(void) {
    uart_init();

    delay_ms(100);   // allow time for device to boot up.
    i2c_init();
    delay_ms(100);   // allow time to settle after init.


    uint8_t dev_addr = lsm6ds33_default_addr;
    uint8_t v = imu_rd(dev_addr, WHO_AM_I);
    if(v != VAL_WHO_AM_I)
        panic("Initial probe failed: expected %x, got %x\n", VAL_WHO_AM_I, v);
    else
        printk("SUCCESS: lsm acknowledged our ping!!\n");

    do_accel_test();
    clean_reboot();
}
