#include "stepper.h"
#include "rpi.h"
#include "math-helpers.h"

stepper_t * stepper_init(unsigned dir, unsigned step){
    stepper_t * stepper = kmalloc(sizeof(stepper_t));
    
	stepper->dir = dir;
	stepper->step = step;
	gpio_set_output(stepper->dir);
	gpio_set_output(stepper->step);

    return stepper;
}

// If you want to do microstep extension:
void stepper_set_microsteps(stepper_t * stepper, stepper_microstep_mode_t microstep_mode){
	switch(microstep_mode) {
		case FULL_STEP:
			printk("Full step\n");
			gpio_write(stepper->MS1, 0);
			gpio_write(stepper->MS2, 0);
			gpio_write(stepper->MS3, 0);
			break;
		case HALF_STEP:
			printk("Half step\n");
			gpio_write(stepper->MS1, 1);
			gpio_write(stepper->MS2, 0);
			gpio_write(stepper->MS3, 0);
			break;
		case QUARTER_STEP:
			printk("Quarter step\n");
			gpio_write(stepper->MS1, 0);
			gpio_write(stepper->MS2, 1);
			gpio_write(stepper->MS3, 0);
			break;
		case EIGHTH_STEP:
			printk("Eighth step\n");
			gpio_write(stepper->MS1, 1);
			gpio_write(stepper->MS2, 1);
			gpio_write(stepper->MS3, 0);
			break;
		case SIXTEENTH_STEP:
			printk("Sixteenth step\n");
			gpio_write(stepper->MS1, 1);
			gpio_write(stepper->MS2, 1);
			gpio_write(stepper->MS3, 1);
			break;
		default:
			printk("Default\n");
			break;
	}
}

// If you want to do microstep extension:
stepper_t * stepper_init_with_microsteps(unsigned dir, unsigned step, unsigned MS1, unsigned MS2, unsigned MS3, stepper_microstep_mode_t microstep_mode){
    stepper_t * stepper = kmalloc(sizeof(stepper_t));
	
	stepper->dir = dir;
	stepper->step = step;
	stepper->MS1 = MS1;
	stepper->MS2 = MS2;
	stepper->MS3 = MS3;

	gpio_set_output(stepper->dir);
	gpio_set_output(stepper->step);
	gpio_set_output(stepper->MS1);
	gpio_set_output(stepper->MS2);
	gpio_set_output(stepper->MS3);

	return stepper;
}

// how many gpio writes should you do?
void stepper_step_forward(stepper_t * stepper){
	gpio_write(stepper->dir, 1);
	gpio_write(stepper->step, 1);
	gpio_write(stepper->step, 0);
    stepper->step_count++;
}

void stepper_step_backward(stepper_t * stepper){
	gpio_write(stepper->dir, 0);
	gpio_write(stepper->step, 1);
	gpio_write(stepper->step, 0);
    stepper->step_count--;
}

int stepper_get_position_in_steps(stepper_t * stepper){
    return stepper->step_count;
}
