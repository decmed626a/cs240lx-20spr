#include "rpi.h"
#include "stepper.h"
#include "stepper-int.h"
#include "math-helpers.h"
#include "i2c.h"
#include "lsm6ds33.h"
#include <limits.h>


// stepper pins
#define DIR 21
#define STEP 20
#define MS1 13
#define MS2 19
#define MS3 26

// play around with this!
#define DELAY_BETWEEN_STEPS_USEC 10000 

void test_stepper(){
    printk("testing stepper\n");
    
    stepper_t * stepper = stepper_init(DIR, STEP);

    for(int i = 0; i < 100; i++){
        assert(stepper_get_position_in_steps(stepper) == i);
        stepper_step_forward(stepper);
        delay_us(DELAY_BETWEEN_STEPS_USEC);
    }

    for(int i = 0; i < 100; i++){
        stepper_step_backward(stepper);
        delay_us(DELAY_BETWEEN_STEPS_USEC);
        assert(stepper_get_position_in_steps(stepper) == 99 - i);
    }

    printk("passed sw checks! How about the hardware?\n");
}

void test_stepper_with_interrupts(){
    printk("now testing stepper w/ interrupts\n");
    stepper_int_t * stepper = stepper_init_with_int(DIR, STEP);

    // test accuracy. Try changing goal and USEC_BETWEEN_STEPS to see how accuracy varies.
    int goal = 200;
    #define USEC_BETWEEN_STEPS 5000
    printk("enqueuing a goal of %d steps, at a rate of one step per %d usec\n", goal, USEC_BETWEEN_STEPS);
    unsigned start_time_usec = timer_get_usec();
    stepper_int_enqueue_pos(stepper, goal, USEC_BETWEEN_STEPS);

    while(!stepper_int_is_free(stepper)){/*wait*/}
    unsigned end_time_usec  = timer_get_usec();
    printk("stepper pos %d. Time it took: %d usec (expected %d, off by: %d)\n", stepper_int_get_position_in_steps(stepper), 
            (end_time_usec - start_time_usec), goal*USEC_BETWEEN_STEPS, goal*USEC_BETWEEN_STEPS - (end_time_usec - start_time_usec));

    // test different speeds
    unsigned num_jobs = 4;
    stepper_position_t * jobs[num_jobs];
    for(int i = 0; i < num_jobs; i++){
        jobs[i] = stepper_int_enqueue_pos(stepper, 100*(i+1) * (i % 2 ? 1 : -1), (num_jobs - i) * 2000);
    }

    for(int i = 0; i < num_jobs; i++){
        while(!stepper_int_position_is_complete(jobs[i])){/*wait*/}
        int pos = stepper_int_get_position_in_steps(stepper);
        printk("job %d done, curr pos %d\n", i, pos);
        assert(pos == 100*(i+1) * (i % 2 ? 1 : -1));
    }
    printk("success!");
}

void test_microstepping(void) {
	printk("now testing microstepping\n");
	stepper_t* stepper = stepper_init_with_microsteps(DIR, STEP, MS1, MS2, MS3, SIXTEENTH_STEP);
    stepper_set_microsteps(stepper, SIXTEENTH_STEP);
	for(int i = 0; i < 1000; i++){
        stepper_step_forward(stepper);
        delay_us(DELAY_BETWEEN_STEPS_USEC);
    }

    for(int i = 0; i < 1000; i++){
        stepper_step_backward(stepper);
        delay_us(DELAY_BETWEEN_STEPS_USEC);
    }
    printk("success!");
}

// const unsigned lsm6ds33_addr = 0b1101011; // this is the gyro/accel;

enum { VAL_WHO_AM_I      = 0x69, };

static imu_xyz_t xyz_mk(int x, int y, int z) {
    return (imu_xyz_t){.x = x, .y = y, .z = z};
}


// read register <reg> from i2c device <addr>
uint8_t imu_rd(uint8_t addr, uint8_t reg) {
    assert(addr);
    assert(reg);
    i2c_write(addr, &reg, 1);
        
    uint8_t v;
    i2c_read(addr,  &v, 1);
    return v;
}

// write register <reg> with value <v> 
void imu_wr(uint8_t addr, uint8_t reg, uint8_t v) {
    assert(addr);
    assert(reg);
    uint8_t data[2];
    data[0] = reg;
    data[1] = v;
    i2c_write(addr, data, 2);
    // printk("writeReg: %x=%x\n", reg, v);
}

// <base_reg> = lowest reg in sequence. --- hw will auto-increment 
// if you set IF_INC during initialization.
int imu_rd_n(uint8_t addr, uint8_t base_reg, uint8_t *v, uint32_t n) {
    assert(addr);
    assert(base_reg);
    i2c_write(addr, &base_reg, 1);
    return i2c_read(addr, v, n);
}

/**********************************************************************
 * simple gyro setup and use.
 */

typedef struct {
    uint8_t addr;
    unsigned hz;
    unsigned dps;
    unsigned scale;
} gyro_t;

static int mdps_raw(uint8_t hi, uint8_t lo) {
	short combo_mg = (hi << 8) | lo;
	return combo_mg;
}

// returns m degree per sec
static int mdps_scaled(int v, int dps_scale) {
	return (dps_scale * v) / 1000;
}


// see p 15 of the datasheet.  confusing we have to do this.
unsigned dps_to_scale(unsigned fs) {
    switch(fs) {
    case 245: return 8750;
    case 500: return 17500;
    case 1000: return 35000;
    case 2000: return 70000;
    default: assert(0);
    }
}

// takes in raw data and scales it.
static imu_xyz_t gyro_scale(gyro_t *h, imu_xyz_t xyz) {
    int s = h->scale;
    int x = mdps_scaled(xyz.x, s);
    int y = mdps_scaled(xyz.y, s);
    int z = mdps_scaled(xyz.z, s);
    return xyz_mk(x,y,z);
}

gyro_t init_gyro(uint8_t addr, lsm6ds33_dps_t dps, lsm6ds33_hz_t hz) {
    dev_barrier();

    if(!legal_dps(dps))
        panic("invalid dps: %x\n", dps);
    if(!legal_gyro_hz(hz)) 
        panic("invalid hz: %x\n", hz);

    // gyroscope turn off / turn on time = 80ms!  app note p22
	// Enable X, Y, and Z output in register CTRL9_XL
	// Put in p. 53 of the datasheet
	imu_wr(addr, CTRL10_C, (1 << 3) | (1 << 4) | (1 << 5));

	delay_ms(80);
	
	// Enable BDU in register CTRL3_C (p. 49 of datasheet)
	// Also need IF_INC enabled of same register
	imu_wr(addr, CTRL3_C, (1 << 6) | (1 << 2));

	// Set to high performance mode, 416Hz
	// Set CTRL1_XL (p. 46)
	imu_wr(addr, CTRL2_G, (hz << 4) | (dps << 2));

	gyro_t gyro_struct = {.addr=addr, .hz=hz, .dps=dps, .scale=dps_to_scale(245)};

	delay_ms(80);

    dev_barrier();

	return gyro_struct;
}

// if GDA of status reg (bit offset = 1) is 1 then there is data.
int gyro_has_data(gyro_t *h) {
    // read from bit 2 of STATUS_REG to see if
	// accelerometer is available, p.56 of datasheet
	unsigned stat_val = imu_rd(h->addr, STATUS_REG) & 2;
	if(stat_val != 0) {
		return 1;
	} else {
		return 0;
	}
}

imu_xyz_t gyro_rd(gyro_t *h) {
    unsigned mdps_scale = h->scale;
    uint8_t addr = h->addr;
	while(!gyro_has_data(h)) {;}

	uint8_t vals[6];

	imu_rd_n(h->addr, OUTX_L_G, vals, 6);

	int x = mdps_raw(vals[1], vals[0]);
	int y = mdps_raw(vals[3], vals[2]);
	int z = mdps_raw(vals[5], vals[4]);

	imu_xyz_t gyro_data = {.x=x, .y=y, .z=z};
	return gyro_data;
}

static void test_dps(int expected, uint8_t h, uint8_t l, int dps_scale) {
    int v = mdps_raw(h,l);
    // the tests are in terms of dps not mdps
    int s = mdps_scaled(v, dps_scale) / 1000;
    printk("raw=%d, scaled=%d, expected=%d\n", v, s, expected);

    // can have issues b/c of roundoff error 
    if(expected != s
    && expected != (s+1)
    && expected != (s-1))
        panic("expected %d, got = %d, scale=%d\n", expected, s, dps_scale);
}

#define MAX_BAND 30000
#define MIN_BAND -30000
#define MAX_THRESHOLD 7000
#define MIN_THRESHOLD -7000
#define WINDOW_SIZE 4

void gyro_motor(void) {
	delay_ms(100);
	i2c_init();
	delay_ms(100);

	uint8_t dev_addr = lsm6ds33_default_addr;
	printk("Dev addr: %d\n", dev_addr);
	printk("WHOAMI: %d\n", WHO_AM_I);
	uint8_t v = imu_rd(dev_addr, WHO_AM_I);
	if(v != VAL_WHO_AM_I) {
		panic("Gyro init failed");
	}

	unsigned scale = dps_to_scale(245);
	test_dps(0, 0x00, 0x00, scale);
	test_dps(100, 0x2c, 0xa4, scale);
	test_dps(200, 0x59, 0x49, scale);
	test_dps(-100, 0xd3, 0x5c, scale);
	test_dps(-200, 0xa6, 0xb7, scale);
	printk("dps_scaling passed\n");

	gyro_t h = init_gyro(lsm6ds33_default_addr, lsm6ds33_245dps, lsm6ds33_208hz);
    
	stepper_t * stepper = stepper_init(DIR, STEP);

	int window[WINDOW_SIZE] = {0, 0, 0, 0};
	int count = 0;
	imu_xyz_t va;
	while(1) {
		while(count < WINDOW_SIZE) {
			va = gyro_rd(&h);
			va = gyro_scale(&h, va);
			window[count] = va.x;
			count++;
		}
	
		va = gyro_rd(&h);
		printk("gyro raw values: x=%d, y=%d, z=%d\n", va.x, va.y, va.z);

		va = gyro_scale(&h, va);
		printk("gyro scaled values: x=%d, y=%d, z=%d\n", va.x, va.y, va.z);

		int temp = (window[0] + window[1] + window[2] + window[3]) / 4;
		
		assert(count == WINDOW_SIZE);
		window[0] = window[1];
		window[1] = window[2];
		window[2] = window[3];
		window[3] = va.x;

		if(temp > MAX_THRESHOLD && (window[3] < window[2])) {
			if(temp > MAX_BAND) {temp = MAX_BAND;}
    		for(int i = 0; i < 100; i++){
        		stepper_step_forward(stepper);
				int floor = MAX_BAND - temp;
				if(floor == 0) { floor = 1;}
       	 		delay_us(floor + 2000);
			}
		}

		if(temp < MIN_THRESHOLD && (window[3] > window[2])) {
			if(temp < MIN_BAND) {temp = MIN_BAND;}
			assert(temp < 0);
    		for(int i = 0; i < 100; i++){
        		stepper_step_backward(stepper);
				int floor = MIN_BAND + temp;
       	 		delay_us(-floor + 2000);
			}
		}

		delay_ms(500);
	}
}

void notmain(){
    uart_init();
    kmalloc_init();
	printk("Stepper: starting\n");

    //test_stepper();

    //test_stepper_with_interrupts();

	//test_microstepping();

	gyro_motor();

	printk("Done!\n");
    clean_reboot();
}
