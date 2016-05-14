
// THIS FILE WAS GENERATED USING chacha_gen.py
// DO NOT MODIFY DIRECTLY

.global chacha_perm_asm
.type chacha_perm_asm, %function

.syntax unified
.cpu cortex-m3
.thumb

// operates on r0 as output and r1 as input pointer
// assumes space of 64 bytes following both pointers

chacha_perm_asm:
push {r0, r4-r12, r14} // preserve r0 + potential other values (r1-r3 are args)

ldm r1!, {r0, r2-r12, r14}
push {r9, r14}  // store 8 and 12 on the stack
ldm r1!, {r9, r14}  // get 13, 14
ldr.W r1, [r1, #0]  // get 15
add r2, r2, r6, ROR #0
eor r9, r9, r2, ROR #0
add r10, r10, r9, ROR #16
eor r6, r6, r10, ROR #0
add r2, r2, r6, ROR #20
eor r9, r9, r2, ROR #16
add r10, r10, r9, ROR #8
eor r6, r6, r10, ROR #12
add r3, r3, r7, ROR #0
eor r14, r14, r3, ROR #0
add r11, r11, r14, ROR #16
eor r7, r7, r11, ROR #0
add r3, r3, r7, ROR #20
eor r14, r14, r3, ROR #16
add r11, r11, r14, ROR #8
eor r7, r7, r11, ROR #12
add r4, r4, r8, ROR #0
eor r1, r1, r4, ROR #0
add r12, r12, r1, ROR #16
eor r8, r8, r12, ROR #0
add r4, r4, r8, ROR #20
eor r1, r1, r4, ROR #16
add r12, r12, r1, ROR #8
eor r8, r8, r12, ROR #12
push {r10, r11}
ldr r10, [sp, #8]
ldr r11, [sp, #12]
add r0, r0, r5, ROR #0
eor r11, r11, r0, ROR #0
add r10, r10, r11, ROR #16
eor r5, r5, r10, ROR #0
add r0, r0, r5, ROR #20
eor r11, r11, r0, ROR #16
add r10, r10, r11, ROR #8
eor r5, r5, r10, ROR #12
add r2, r2, r7, ROR #13
eor r11, r11, r2, ROR #24
add r12, r12, r11, ROR #24
eor r7, r7, r12, ROR #19
add r2, r2, r7, ROR #1
eor r11, r11, r2, ROR #8
add r12, r12, r11, ROR #16
eor r7, r7, r12, ROR #31
add r3, r3, r8, ROR #13
eor r9, r9, r3, ROR #24
add r10, r10, r9, ROR #24
eor r8, r8, r10, ROR #19
add r3, r3, r8, ROR #1
eor r9, r9, r3, ROR #8
add r10, r10, r9, ROR #16
eor r8, r8, r10, ROR #31
str r10, [sp, #8]
str r11, [sp, #12]
pop {r10, r11}
add r0, r0, r6, ROR #13
eor r1, r1, r0, ROR #24
add r11, r11, r1, ROR #24
eor r6, r6, r11, ROR #19
add r0, r0, r6, ROR #1
eor r1, r1, r0, ROR #8
add r11, r11, r1, ROR #16
eor r6, r6, r11, ROR #31
add r4, r4, r5, ROR #13
eor r14, r14, r4, ROR #24
add r10, r10, r14, ROR #24
eor r5, r5, r10, ROR #19
add r4, r4, r5, ROR #1
eor r14, r14, r4, ROR #8
add r10, r10, r14, ROR #16
eor r5, r5, r10, ROR #31
add r2, r2, r6, ROR #26
eor r9, r9, r2, ROR #16
add r10, r10, r9, ROR #0
eor r6, r6, r10, ROR #6
add r2, r2, r6, ROR #14
eor r9, r9, r2, ROR #0
add r10, r10, r9, ROR #24
eor r6, r6, r10, ROR #18
add r3, r3, r7, ROR #26
eor r14, r14, r3, ROR #16
add r11, r11, r14, ROR #0
eor r7, r7, r11, ROR #6
add r3, r3, r7, ROR #14
eor r14, r14, r3, ROR #0
add r11, r11, r14, ROR #24
eor r7, r7, r11, ROR #18
add r4, r4, r8, ROR #26
eor r1, r1, r4, ROR #16
add r12, r12, r1, ROR #0
eor r8, r8, r12, ROR #6
add r4, r4, r8, ROR #14
eor r1, r1, r4, ROR #0
add r12, r12, r1, ROR #24
eor r8, r8, r12, ROR #18
push {r10, r11}
ldr r10, [sp, #8]
ldr r11, [sp, #12]
add r0, r0, r5, ROR #26
eor r11, r11, r0, ROR #16
add r10, r10, r11, ROR #0
eor r5, r5, r10, ROR #6
add r0, r0, r5, ROR #14
eor r11, r11, r0, ROR #0
add r10, r10, r11, ROR #24
eor r5, r5, r10, ROR #18
add r2, r2, r7, ROR #7
eor r11, r11, r2, ROR #8
add r12, r12, r11, ROR #8
eor r7, r7, r12, ROR #25
add r2, r2, r7, ROR #27
eor r11, r11, r2, ROR #24
add r12, r12, r11, ROR #0
eor r7, r7, r12, ROR #5
add r3, r3, r8, ROR #7
eor r9, r9, r3, ROR #8
add r10, r10, r9, ROR #8
eor r8, r8, r10, ROR #25
add r3, r3, r8, ROR #27
eor r9, r9, r3, ROR #24
add r10, r10, r9, ROR #0
eor r8, r8, r10, ROR #5
str r10, [sp, #8]
str r11, [sp, #12]
pop {r10, r11}
add r0, r0, r6, ROR #7
eor r1, r1, r0, ROR #8
add r11, r11, r1, ROR #8
eor r6, r6, r11, ROR #25
add r0, r0, r6, ROR #27
eor r1, r1, r0, ROR #24
add r11, r11, r1, ROR #0
eor r6, r6, r11, ROR #5
add r4, r4, r5, ROR #7
eor r14, r14, r4, ROR #8
add r10, r10, r14, ROR #8
eor r5, r5, r10, ROR #25
add r4, r4, r5, ROR #27
eor r14, r14, r4, ROR #24
add r10, r10, r14, ROR #0
eor r5, r5, r10, ROR #5
add r2, r2, r6, ROR #20
eor r9, r9, r2, ROR #0
add r10, r10, r9, ROR #16
eor r6, r6, r10, ROR #12
add r2, r2, r6, ROR #8
eor r9, r9, r2, ROR #16
add r10, r10, r9, ROR #8
eor r6, r6, r10, ROR #24
add r3, r3, r7, ROR #20
eor r14, r14, r3, ROR #0
add r11, r11, r14, ROR #16
eor r7, r7, r11, ROR #12
add r3, r3, r7, ROR #8
eor r14, r14, r3, ROR #16
add r11, r11, r14, ROR #8
eor r7, r7, r11, ROR #24
add r4, r4, r8, ROR #20
eor r1, r1, r4, ROR #0
add r12, r12, r1, ROR #16
eor r8, r8, r12, ROR #12
add r4, r4, r8, ROR #8
eor r1, r1, r4, ROR #16
add r12, r12, r1, ROR #8
eor r8, r8, r12, ROR #24
push {r10, r11}
ldr r10, [sp, #8]
ldr r11, [sp, #12]
add r0, r0, r5, ROR #20
eor r11, r11, r0, ROR #0
add r10, r10, r11, ROR #16
eor r5, r5, r10, ROR #12
add r0, r0, r5, ROR #8
eor r11, r11, r0, ROR #16
add r10, r10, r11, ROR #8
eor r5, r5, r10, ROR #24
add r2, r2, r7, ROR #1
eor r11, r11, r2, ROR #24
add r12, r12, r11, ROR #24
eor r7, r7, r12, ROR #31
add r2, r2, r7, ROR #21
eor r11, r11, r2, ROR #8
add r12, r12, r11, ROR #16
eor r7, r7, r12, ROR #11
add r3, r3, r8, ROR #1
eor r9, r9, r3, ROR #24
add r10, r10, r9, ROR #24
eor r8, r8, r10, ROR #31
add r3, r3, r8, ROR #21
eor r9, r9, r3, ROR #8
add r10, r10, r9, ROR #16
eor r8, r8, r10, ROR #11
str r10, [sp, #8]
str r11, [sp, #12]
pop {r10, r11}
add r0, r0, r6, ROR #1
eor r1, r1, r0, ROR #24
add r11, r11, r1, ROR #24
eor r6, r6, r11, ROR #31
add r0, r0, r6, ROR #21
eor r1, r1, r0, ROR #8
add r11, r11, r1, ROR #16
eor r6, r6, r11, ROR #11
add r4, r4, r5, ROR #1
eor r14, r14, r4, ROR #24
add r10, r10, r14, ROR #24
eor r5, r5, r10, ROR #31
add r4, r4, r5, ROR #21
eor r14, r14, r4, ROR #8
add r10, r10, r14, ROR #16
eor r5, r5, r10, ROR #11
add r2, r2, r6, ROR #14
eor r9, r9, r2, ROR #16
add r10, r10, r9, ROR #0
eor r6, r6, r10, ROR #18
add r2, r2, r6, ROR #2
eor r9, r9, r2, ROR #0
add r10, r10, r9, ROR #24
eor r6, r6, r10, ROR #30
add r3, r3, r7, ROR #14
eor r14, r14, r3, ROR #16
add r11, r11, r14, ROR #0
eor r7, r7, r11, ROR #18
add r3, r3, r7, ROR #2
eor r14, r14, r3, ROR #0
add r11, r11, r14, ROR #24
eor r7, r7, r11, ROR #30
add r4, r4, r8, ROR #14
eor r1, r1, r4, ROR #16
add r12, r12, r1, ROR #0
eor r8, r8, r12, ROR #18
add r4, r4, r8, ROR #2
eor r1, r1, r4, ROR #0
add r12, r12, r1, ROR #24
eor r8, r8, r12, ROR #30
push {r10, r11}
ldr r10, [sp, #8]
ldr r11, [sp, #12]
add r0, r0, r5, ROR #14
eor r11, r11, r0, ROR #16
add r10, r10, r11, ROR #0
eor r5, r5, r10, ROR #18
add r0, r0, r5, ROR #2
eor r11, r11, r0, ROR #0
add r10, r10, r11, ROR #24
eor r5, r5, r10, ROR #30
add r2, r2, r7, ROR #27
eor r11, r11, r2, ROR #8
add r12, r12, r11, ROR #8
eor r7, r7, r12, ROR #5
add r2, r2, r7, ROR #15
eor r11, r11, r2, ROR #24
add r12, r12, r11, ROR #0
eor r7, r7, r12, ROR #17
add r3, r3, r8, ROR #27
eor r9, r9, r3, ROR #8
add r10, r10, r9, ROR #8
eor r8, r8, r10, ROR #5
add r3, r3, r8, ROR #15
eor r9, r9, r3, ROR #24
add r10, r10, r9, ROR #0
eor r8, r8, r10, ROR #17
str r10, [sp, #8]
str r11, [sp, #12]
pop {r10, r11}
add r0, r0, r6, ROR #27
eor r1, r1, r0, ROR #8
add r11, r11, r1, ROR #8
eor r6, r6, r11, ROR #5
add r0, r0, r6, ROR #15
eor r1, r1, r0, ROR #24
add r11, r11, r1, ROR #0
eor r6, r6, r11, ROR #17
add r4, r4, r5, ROR #27
eor r14, r14, r4, ROR #8
add r10, r10, r14, ROR #8
eor r5, r5, r10, ROR #5
add r4, r4, r5, ROR #15
eor r14, r14, r4, ROR #24
add r10, r10, r14, ROR #0
eor r5, r5, r10, ROR #17
add r2, r2, r6, ROR #8
eor r9, r9, r2, ROR #0
add r10, r10, r9, ROR #16
eor r6, r6, r10, ROR #24
add r2, r2, r6, ROR #28
eor r9, r9, r2, ROR #16
add r10, r10, r9, ROR #8
eor r6, r6, r10, ROR #4
add r3, r3, r7, ROR #8
eor r14, r14, r3, ROR #0
add r11, r11, r14, ROR #16
eor r7, r7, r11, ROR #24
add r3, r3, r7, ROR #28
eor r14, r14, r3, ROR #16
add r11, r11, r14, ROR #8
eor r7, r7, r11, ROR #4
add r4, r4, r8, ROR #8
eor r1, r1, r4, ROR #0
add r12, r12, r1, ROR #16
eor r8, r8, r12, ROR #24
add r4, r4, r8, ROR #28
eor r1, r1, r4, ROR #16
add r12, r12, r1, ROR #8
eor r8, r8, r12, ROR #4
push {r10, r11}
ldr r10, [sp, #8]
ldr r11, [sp, #12]
add r0, r0, r5, ROR #8
eor r11, r11, r0, ROR #0
add r10, r10, r11, ROR #16
eor r5, r5, r10, ROR #24
add r0, r0, r5, ROR #28
eor r11, r11, r0, ROR #16
add r10, r10, r11, ROR #8
eor r5, r5, r10, ROR #4
add r2, r2, r7, ROR #21
eor r11, r11, r2, ROR #24
add r12, r12, r11, ROR #24
eor r7, r7, r12, ROR #11
add r2, r2, r7, ROR #9
eor r11, r11, r2, ROR #8
add r12, r12, r11, ROR #16
eor r7, r7, r12, ROR #23
add r3, r3, r8, ROR #21
eor r9, r9, r3, ROR #24
add r10, r10, r9, ROR #24
eor r8, r8, r10, ROR #11
add r3, r3, r8, ROR #9
eor r9, r9, r3, ROR #8
add r10, r10, r9, ROR #16
eor r8, r8, r10, ROR #23
str r10, [sp, #8]
str r11, [sp, #12]
pop {r10, r11}
add r0, r0, r6, ROR #21
eor r1, r1, r0, ROR #24
add r11, r11, r1, ROR #24
eor r6, r6, r11, ROR #11
add r0, r0, r6, ROR #9
eor r1, r1, r0, ROR #8
add r11, r11, r1, ROR #16
eor r6, r6, r11, ROR #23
add r4, r4, r5, ROR #21
eor r14, r14, r4, ROR #24
add r10, r10, r14, ROR #24
eor r5, r5, r10, ROR #11
add r4, r4, r5, ROR #9
eor r14, r14, r4, ROR #8
add r10, r10, r14, ROR #16
eor r5, r5, r10, ROR #23
add r2, r2, r6, ROR #2
eor r9, r9, r2, ROR #16
add r10, r10, r9, ROR #0
eor r6, r6, r10, ROR #30
add r2, r2, r6, ROR #22
eor r9, r9, r2, ROR #0
add r10, r10, r9, ROR #24
eor r6, r6, r10, ROR #10
add r3, r3, r7, ROR #2
eor r14, r14, r3, ROR #16
add r11, r11, r14, ROR #0
eor r7, r7, r11, ROR #30
add r3, r3, r7, ROR #22
eor r14, r14, r3, ROR #0
add r11, r11, r14, ROR #24
eor r7, r7, r11, ROR #10
add r4, r4, r8, ROR #2
eor r1, r1, r4, ROR #16
add r12, r12, r1, ROR #0
eor r8, r8, r12, ROR #30
add r4, r4, r8, ROR #22
eor r1, r1, r4, ROR #0
add r12, r12, r1, ROR #24
eor r8, r8, r12, ROR #10
push {r10, r11}
ldr r10, [sp, #8]
ldr r11, [sp, #12]
add r0, r0, r5, ROR #2
eor r11, r11, r0, ROR #16
add r10, r10, r11, ROR #0
eor r5, r5, r10, ROR #30
add r0, r0, r5, ROR #22
eor r11, r11, r0, ROR #0
add r10, r10, r11, ROR #24
eor r5, r5, r10, ROR #10
add r2, r2, r7, ROR #15
eor r11, r11, r2, ROR #8
add r12, r12, r11, ROR #8
eor r7, r7, r12, ROR #17
add r2, r2, r7, ROR #3
eor r11, r11, r2, ROR #24
add r12, r12, r11, ROR #0
eor r7, r7, r12, ROR #29
add r3, r3, r8, ROR #15
eor r9, r9, r3, ROR #8
add r10, r10, r9, ROR #8
eor r8, r8, r10, ROR #17
add r3, r3, r8, ROR #3
eor r9, r9, r3, ROR #24
add r10, r10, r9, ROR #0
eor r8, r8, r10, ROR #29
str r10, [sp, #8]
str r11, [sp, #12]
pop {r10, r11}
add r0, r0, r6, ROR #15
eor r1, r1, r0, ROR #8
add r11, r11, r1, ROR #8
eor r6, r6, r11, ROR #17
add r0, r0, r6, ROR #3
eor r1, r1, r0, ROR #24
add r11, r11, r1, ROR #0
eor r6, r6, r11, ROR #29
add r4, r4, r5, ROR #15
eor r14, r14, r4, ROR #8
add r10, r10, r14, ROR #8
eor r5, r5, r10, ROR #17
add r4, r4, r5, ROR #3
eor r14, r14, r4, ROR #24
add r10, r10, r14, ROR #0
eor r5, r5, r10, ROR #29
add r2, r2, r6, ROR #28
eor r9, r9, r2, ROR #0
add r10, r10, r9, ROR #16
eor r6, r6, r10, ROR #4
add r2, r2, r6, ROR #16
eor r9, r9, r2, ROR #16
add r10, r10, r9, ROR #8
eor r6, r6, r10, ROR #16
add r3, r3, r7, ROR #28
eor r14, r14, r3, ROR #0
add r11, r11, r14, ROR #16
eor r7, r7, r11, ROR #4
add r3, r3, r7, ROR #16
eor r14, r14, r3, ROR #16
add r11, r11, r14, ROR #8
eor r7, r7, r11, ROR #16
add r4, r4, r8, ROR #28
eor r1, r1, r4, ROR #0
add r12, r12, r1, ROR #16
eor r8, r8, r12, ROR #4
add r4, r4, r8, ROR #16
eor r1, r1, r4, ROR #16
add r12, r12, r1, ROR #8
eor r8, r8, r12, ROR #16
push {r10, r11}
ldr r10, [sp, #8]
ldr r11, [sp, #12]
add r0, r0, r5, ROR #28
eor r11, r11, r0, ROR #0
add r10, r10, r11, ROR #16
eor r5, r5, r10, ROR #4
add r0, r0, r5, ROR #16
eor r11, r11, r0, ROR #16
add r10, r10, r11, ROR #8
eor r5, r5, r10, ROR #16
add r2, r2, r7, ROR #9
eor r11, r11, r2, ROR #24
add r12, r12, r11, ROR #24
eor r7, r7, r12, ROR #23
add r2, r2, r7, ROR #29
eor r11, r11, r2, ROR #8
add r12, r12, r11, ROR #16
eor r7, r7, r12, ROR #3
add r3, r3, r8, ROR #9
eor r9, r9, r3, ROR #24
add r10, r10, r9, ROR #24
eor r8, r8, r10, ROR #23
add r3, r3, r8, ROR #29
eor r9, r9, r3, ROR #8
add r10, r10, r9, ROR #16
eor r8, r8, r10, ROR #3
str r10, [sp, #8]
str r11, [sp, #12]
pop {r10, r11}
add r0, r0, r6, ROR #9
eor r1, r1, r0, ROR #24
add r11, r11, r1, ROR #24
eor r6, r6, r11, ROR #23
add r0, r0, r6, ROR #29
eor r1, r1, r0, ROR #8
add r11, r11, r1, ROR #16
eor r6, r6, r11, ROR #3
add r4, r4, r5, ROR #9
eor r14, r14, r4, ROR #24
add r10, r10, r14, ROR #24
eor r5, r5, r10, ROR #23
add r4, r4, r5, ROR #29
eor r14, r14, r4, ROR #8
add r10, r10, r14, ROR #16
eor r5, r5, r10, ROR #3
add r2, r2, r6, ROR #22
eor r9, r9, r2, ROR #16
add r10, r10, r9, ROR #0
eor r6, r6, r10, ROR #10
add r2, r2, r6, ROR #10
eor r9, r9, r2, ROR #0
add r10, r10, r9, ROR #24
eor r6, r6, r10, ROR #22
add r3, r3, r7, ROR #22
eor r14, r14, r3, ROR #16
add r11, r11, r14, ROR #0
eor r7, r7, r11, ROR #10
add r3, r3, r7, ROR #10
eor r14, r14, r3, ROR #0
add r11, r11, r14, ROR #24
eor r7, r7, r11, ROR #22
add r4, r4, r8, ROR #22
eor r1, r1, r4, ROR #16
add r12, r12, r1, ROR #0
eor r8, r8, r12, ROR #10
add r4, r4, r8, ROR #10
eor r1, r1, r4, ROR #0
add r12, r12, r1, ROR #24
eor r8, r8, r12, ROR #22
push {r10, r11}
ldr r10, [sp, #8]
ldr r11, [sp, #12]
add r0, r0, r5, ROR #22
eor r11, r11, r0, ROR #16
add r10, r10, r11, ROR #0
eor r5, r5, r10, ROR #10
add r0, r0, r5, ROR #10
eor r11, r11, r0, ROR #0
add r10, r10, r11, ROR #24
eor r5, r5, r10, ROR #22
add r2, r2, r7, ROR #3
eor r11, r11, r2, ROR #8
add r12, r12, r11, ROR #8
eor r7, r7, r12, ROR #29
add r2, r2, r7, ROR #23
eor r11, r11, r2, ROR #24
add r12, r12, r11, ROR #0
eor r7, r7, r12, ROR #9
add r3, r3, r8, ROR #3
eor r9, r9, r3, ROR #8
add r10, r10, r9, ROR #8
eor r8, r8, r10, ROR #29
add r3, r3, r8, ROR #23
eor r9, r9, r3, ROR #24
add r10, r10, r9, ROR #0
eor r8, r8, r10, ROR #9
str r10, [sp, #8]
str r11, [sp, #12]
pop {r10, r11}
add r0, r0, r6, ROR #3
eor r1, r1, r0, ROR #8
add r11, r11, r1, ROR #8
eor r6, r6, r11, ROR #29
add r0, r0, r6, ROR #23
eor r1, r1, r0, ROR #24
add r11, r11, r1, ROR #0
eor r6, r6, r11, ROR #9
add r4, r4, r5, ROR #3
eor r14, r14, r4, ROR #8
add r10, r10, r14, ROR #8
eor r5, r5, r10, ROR #29
add r4, r4, r5, ROR #23
eor r14, r14, r4, ROR #24
add r10, r10, r14, ROR #0
eor r5, r5, r10, ROR #9
add r2, r2, r6, ROR #16
eor r9, r9, r2, ROR #0
add r10, r10, r9, ROR #16
eor r6, r6, r10, ROR #16
add r2, r2, r6, ROR #4
eor r9, r9, r2, ROR #16
add r10, r10, r9, ROR #8
eor r6, r6, r10, ROR #28
add r3, r3, r7, ROR #16
eor r14, r14, r3, ROR #0
add r11, r11, r14, ROR #16
eor r7, r7, r11, ROR #16
add r3, r3, r7, ROR #4
eor r14, r14, r3, ROR #16
add r11, r11, r14, ROR #8
eor r7, r7, r11, ROR #28
add r4, r4, r8, ROR #16
eor r1, r1, r4, ROR #0
add r12, r12, r1, ROR #16
eor r8, r8, r12, ROR #16
add r4, r4, r8, ROR #4
eor r1, r1, r4, ROR #16
add r12, r12, r1, ROR #8
eor r8, r8, r12, ROR #28
push {r10, r11}
ldr r10, [sp, #8]
ldr r11, [sp, #12]
add r0, r0, r5, ROR #16
eor r11, r11, r0, ROR #0
add r10, r10, r11, ROR #16
eor r5, r5, r10, ROR #16
add r0, r0, r5, ROR #4
eor r11, r11, r0, ROR #16
add r10, r10, r11, ROR #8
eor r5, r5, r10, ROR #28
add r2, r2, r7, ROR #29
eor r11, r11, r2, ROR #24
add r12, r12, r11, ROR #24
eor r7, r7, r12, ROR #3
add r2, r2, r7, ROR #17
eor r11, r11, r2, ROR #8
add r12, r12, r11, ROR #16
eor r7, r7, r12, ROR #15
add r3, r3, r8, ROR #29
eor r9, r9, r3, ROR #24
add r10, r10, r9, ROR #24
eor r8, r8, r10, ROR #3
add r3, r3, r8, ROR #17
eor r9, r9, r3, ROR #8
add r10, r10, r9, ROR #16
eor r8, r8, r10, ROR #15
str r10, [sp, #8]
str r11, [sp, #12]
pop {r10, r11}
add r0, r0, r6, ROR #29
eor r1, r1, r0, ROR #24
add r11, r11, r1, ROR #24
eor r6, r6, r11, ROR #3
add r0, r0, r6, ROR #17
eor r1, r1, r0, ROR #8
add r11, r11, r1, ROR #16
eor r6, r6, r11, ROR #15
add r4, r4, r5, ROR #29
eor r14, r14, r4, ROR #24
add r10, r10, r14, ROR #24
eor r5, r5, r10, ROR #3
add r4, r4, r5, ROR #17
eor r14, r14, r4, ROR #8
add r10, r10, r14, ROR #16
eor r5, r5, r10, ROR #15
add r2, r2, r6, ROR #10
eor r9, r9, r2, ROR #16
add r10, r10, r9, ROR #0
eor r6, r6, r10, ROR #22
add r2, r2, r6, ROR #30
eor r9, r9, r2, ROR #0
add r10, r10, r9, ROR #24
eor r6, r6, r10, ROR #2
add r3, r3, r7, ROR #10
eor r14, r14, r3, ROR #16
add r11, r11, r14, ROR #0
eor r7, r7, r11, ROR #22
add r3, r3, r7, ROR #30
eor r14, r14, r3, ROR #0
add r11, r11, r14, ROR #24
eor r7, r7, r11, ROR #2
add r4, r4, r8, ROR #10
eor r1, r1, r4, ROR #16
add r12, r12, r1, ROR #0
eor r8, r8, r12, ROR #22
add r4, r4, r8, ROR #30
eor r1, r1, r4, ROR #0
add r12, r12, r1, ROR #24
eor r8, r8, r12, ROR #2
push {r10, r11}
ldr r10, [sp, #8]
ldr r11, [sp, #12]
add r0, r0, r5, ROR #10
eor r11, r11, r0, ROR #16
add r10, r10, r11, ROR #0
eor r5, r5, r10, ROR #22
add r0, r0, r5, ROR #30
eor r11, r11, r0, ROR #0
add r10, r10, r11, ROR #24
eor r5, r5, r10, ROR #2
add r2, r2, r7, ROR #23
eor r11, r11, r2, ROR #8
add r12, r12, r11, ROR #8
eor r7, r7, r12, ROR #9
add r2, r2, r7, ROR #11
eor r11, r11, r2, ROR #24
add r12, r12, r11, ROR #0
eor r7, r7, r12, ROR #21
add r3, r3, r8, ROR #23
eor r9, r9, r3, ROR #8
add r10, r10, r9, ROR #8
eor r8, r8, r10, ROR #9
add r3, r3, r8, ROR #11
eor r9, r9, r3, ROR #24
add r10, r10, r9, ROR #0
eor r8, r8, r10, ROR #21
str r10, [sp, #8]
str r11, [sp, #12]
pop {r10, r11}
add r0, r0, r6, ROR #23
eor r1, r1, r0, ROR #8
add r11, r11, r1, ROR #8
eor r6, r6, r11, ROR #9
add r0, r0, r6, ROR #11
eor r1, r1, r0, ROR #24
add r11, r11, r1, ROR #0
eor r6, r6, r11, ROR #21
add r4, r4, r5, ROR #23
eor r14, r14, r4, ROR #8
add r10, r10, r14, ROR #8
eor r5, r5, r10, ROR #9
add r4, r4, r5, ROR #11
eor r14, r14, r4, ROR #24
add r10, r10, r14, ROR #0
eor r5, r5, r10, ROR #21
ror r5, r5, #4
ror r6, r6, #4
ror r7, r7, #4
ror r8, r8, #4

push {r1} // save the value in r1 to make room for the output address
ldr r1, [sp, #12] // load the output address back to r1
stm r1!, {r0, r2-r8}
ldm sp, {r2-r4} // load three registers that are temporarily on the stack
stm r1!, {r3, r10-r12}
stm r1!, {r4, r9, r14}
str r2, [r1, #0]

add sp, #16
pop {r4-r12, r14} // restore potential other values
bx lr
