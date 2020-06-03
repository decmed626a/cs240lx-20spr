#include "stepper-int.h"
#include "timer-interrupt.h"
#include "cycle-count.h"

// you can/should play around with this
#define STEPPER_INT_TIMER_INT_PERIOD 100 

static int first_init = 1;

#define MAX_STEPPERS 16
static stepper_int_t * my_steppers[MAX_STEPPERS];
static unsigned num_steppers = 0;

void stepper_int_handler(unsigned pc) {
    // check and clear timer interrupt
    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;
    PUT32(arm_timer_IRQClear, 1);
    dev_barrier();  

	uint32_t curr_time = timer_get_usec();
	for(int i = 0; i < num_steppers; i++) {
		stepper_int_t* stepper = my_steppers[i];
		if(!Q_empty(&(stepper->positions_Q))) {
			stepper_position_t* goal = Q_start(&(stepper->positions_Q));
			int curr_pos = stepper_int_get_position_in_steps(stepper);
			if(goal->goal_steps == curr_pos) {
				stepper_position_t* done = Q_pop(&(stepper->positions_Q));
				done->status = FINISHED;
			} else if(curr_time > (goal->usec_between_steps + goal->usec_at_prev_step)) {
				if(curr_pos - goal->goal_steps < 0) {
					stepper_step_forward(stepper->stepper);
				} else {
					stepper_step_backward(stepper->stepper);
				}
				goal->usec_at_prev_step = curr_time;
			}
		} else {
			stepper->status = NOT_IN_JOB;
		}
	}
}

void interrupt_vector(unsigned pc){
    stepper_int_handler(pc);
}

stepper_int_t * stepper_init_with_int(unsigned dir, unsigned step){
    if(num_steppers == MAX_STEPPERS){
        return NULL;
    }
    //kmalloc_init();

	stepper_int_t* stepper = kmalloc(sizeof(stepper_int_t));
	stepper->status = NOT_IN_JOB;
	stepper->stepper = stepper_init(dir, step);
	my_steppers[num_steppers] = stepper;
	num_steppers++;

    //initialize interrupts; only do once, on the first init
    if(first_init){
        first_init = 0;
        int_init();
        cycle_cnt_init();
        timer_interrupt_init(STEPPER_INT_TIMER_INT_PERIOD);
        system_enable_interrupts();
    }
    return stepper;
}

stepper_int_t * stepper_int_init_with_microsteps(unsigned dir, unsigned step, unsigned MS1, unsigned MS2, unsigned MS3, stepper_microstep_mode_t microstep_mode){
    unimplemented();
}

/* retuns the enqueued position. perhaps return the queue of positions instead? */
stepper_position_t * stepper_int_enqueue_pos(stepper_int_t * stepper, int goal_steps, unsigned usec_between_steps){
	stepper->status = IN_JOB;
	stepper_position_t* new_pos = kmalloc(sizeof(stepper_position_t));
	new_pos->status = NOT_STARTED;
	new_pos->goal_steps = goal_steps;
	new_pos->usec_between_steps = usec_between_steps;
	new_pos->usec_at_prev_step = 0;
	Q_append(&(stepper->positions_Q), new_pos);

    return new_pos;
}

int stepper_int_get_position_in_steps(stepper_int_t * stepper){
    return stepper_get_position_in_steps(stepper->stepper);
}

int stepper_int_is_free(stepper_int_t * stepper){
    return stepper->status == NOT_IN_JOB;
}

int stepper_int_position_is_complete(stepper_position_t * pos){
    return pos->status == FINISHED;
}

