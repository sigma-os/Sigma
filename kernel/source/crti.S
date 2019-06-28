.section .init_start
.global _init
.type _init, @function
_init:
    push %rbp
    movq %rsp, %rbp

.section .fini_start
.global _fini
.type _fini, @function
_fini:
    push %rbp
    movq %rsp, %rbp
