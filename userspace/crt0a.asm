extern __crt0_run

section .text

global _start
_start:
    call __crt0_run

_hang:
    jmp _hang