/*
 * interrupt-asm.s
 *
 * Code for interrupt handling.  Refer to armisa.pdf in docs/ for what
 * the opcodes mean.
 */
#include "rpi-asm.h"

.globl _interrupt_table
.globl _interrupt_table_end
_interrupt_table:
  ldr pc, _reset_asm
  ldr pc, _undefined_instruction_asm
  ldr pc, _software_interrupt_asm
  ldr pc, _prefetch_abort_asm
  ldr pc, _data_abort_asm
  ldr pc, _reset_asm
  ldr pc, _interrupt_asm
fast_interrupt_asm:
  mov sp, #INT_STACK_ADDR 
  sub   lr, lr, #4
  bl    fast_interrupt_vector  

_reset_asm:                   .word reset_asm
_undefined_instruction_asm:   .word undefined_instruction_asm
_software_interrupt_asm:      .word software_interrupt_asm
_prefetch_abort_asm:          .word prefetch_abort_asm
_data_abort_asm:              .word data_abort_asm
_interrupt_asm:               .word interrupt_asm
_interrupt_table_end:

@ only handler that should run since we only enable general interrupts
interrupt_asm:
  mov sp, #INT_STACK_ADDR   @ i believe we have 512mb - 16mb, so this should be safe
  sub   lr, lr, #4

  push  {r0-r12,lr}         @ XXX: pushing too many registers: only need caller
  @ vpush {s0-s15}	        @ uncomment if want to save caller-saved fp regs

  mov   r0, lr              @ Pass old pc
  @ bl    interrupt_vector    @ C function

  @ vpop {s0-s15}           @ pop caller saved fp regs
  pop   {r0-r12,lr} 	    @ pop integer registers

  @ return from interrupt handler: will re-enable general ints.
  movs    pc, lr        @ moves the link register into the pc and implicitly
                        @ loads the PC with the result, then copies the 
                        @ SPSR to the CPSR.


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ these handlers simply load a value into the stack pointer and call
@ the default handle for each exception, which will promptly panic and 
@ reboot.
@
@ you need to look up in the arm pdf what the state of lr is for each.
@ sometimes you need to subtact 4, sometimes 8.
@
@ you should override these as you need them.
reset_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #4
  bl    reset_vector


#define TRAMPOLINE(fn)              \
    mov sp, #INT_STACK_ADDR;          \
    push  {r0-r12,lr};                \
    mov   r0, lr;                     \
    bl    fn;                         \
    pop   {r0-r12,lr};                \
    movs    pc, lr
    
MK_FN(data_abort_asm)
    sub   lr, lr, #8
    TRAMPOLINE(data_abort_vector)
    
MK_FN(prefetch_abort_asm)
    sub   lr, lr, #4
    TRAMPOLINE(prefetch_abort_vector)

undefined_instruction_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #4
  bl    undefined_instruction_vector
software_interrupt_asm:
  push  {r4-r12,lr}         @ XXX: pushing too many registers: only need caller
  sub   lr, #4              @ b/c lr is pointing 4 too far... 
  mov   r1, r0              @ Pass r0 
  mov   r0, lr              @ Pass old pc

  @bl    syscall_vector    @ C function, possibly interrupt_vector

  pop   {r4-r12,lr} 	    @ pop integer registers
  
  movs pc, lr @ special thing that also transfers (saved) spsr to (current) cpsr.

@ prefetch_abort_asm:
@   mov sp, #INT_STACK_ADDR   
@   sub   lr, lr, #4
@   bl    prefetch_abort_vector
@ data_abort_asm:
@   mov sp, #INT_STACK_ADDR   
@   sub   lr, lr, #4
@   bl    data_abort_vector

/********
lab 16 patch
 */

// switches mode, jumps to routine passed in and doesn't return
@ arguments:
@   r0 = the cpsr value to set.
@   r1 = the routine to call
@   r2 = the handle pointer to pass to the call <r1>
@ also: move the current mode's stack pointer to the next mode's
@ sp
@
@ maybe we should just have a stack pointer switch routine?   the
@ trouble with this is that we need to do a system call to change
@ to super mode.  i don't know if this is the right thing.
MK_FN(user_trampoline_ret)
  @ Save the old mode and sp
  push {r4-r12, lr}
  mrs r4, cpsr
  
  @ Load in new mode and prefetch flush
  msr cpsr, r0
  mcr p15, 0, r0, c7, c5, 4 @ prefetch flush
  mov sp, #STACK_ADDR2;          \
  
  mov r0, r2 @ give arg to routine

  blx r1

  @ return to old mode
  mov r0, r4 @ pass old mode as arg
  SWI 1 

  pop {r4-r12, lr}

  bx lr

MK_FN(sys_try_lock_asm)
  push {r4-r12, lr}
  swi 2
  pop {r4-r12, lr}
  bx lr

MK_FN(sys_foo_asm)
  push {r4-r12, lr}
  swi 3
  pop {r4-r12, lr}
  bx lr

MK_FN(run_fn_helper)
  @ Save the old mode and sp
  push {r4-r12, lr}
  mrs r4, cpsr
  
  @ Load in new mode and prefetch flush
  msr cpsr, r0
  mcr p15, 0, r0, c7, c5, 4 @ prefetch flush
  mov sp, r2
  
  mov r0, r2
  
  ldr r1, [r1]

  blx r1
  
  mov r5, r0

  @ return to old mode
  
  mov r0, r4 @ pass old mode as arg
 
  SWI 1 

  mov r0, r5

  pop {r4-r12, lr}

  bx lr
