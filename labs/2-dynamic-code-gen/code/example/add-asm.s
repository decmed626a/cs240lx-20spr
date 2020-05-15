add r0, r0, r1
bx lr
mov r0, #1
nop

.global foo
foo:
    bx lr
