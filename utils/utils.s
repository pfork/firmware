
/* vectors.s */
.cpu cortex-m3
.thumb

;@-----------------------
.thumb_func
.globl PUT8
PUT8:
    strb r1,[r0]
    bx lr
;@-----------------------
.thumb_func
.globl PUT16
PUT16:
    strh r1,[r0]
    bx lr
;@-----------------------
.thumb_func
.globl PUT32
PUT32:
    str r1,[r0]
    bx lr
;@-----------------------
.thumb_func
.globl GET32
GET32:
    ldr r0,[r0]
    bx lr
;@-----------------------
.thumb_func
.globl GET16
GET16:
    ldrh r0,[r0]
    bx lr
.thumb_func
.globl ASM_DELAY
ASM_DELAY:
    sub r0,#1 ;@ subs r0,#1
    bne ASM_DELAY
    bx lr

.end
