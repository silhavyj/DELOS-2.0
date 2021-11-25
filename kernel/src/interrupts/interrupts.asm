;   Inspiration: http://www.osdever.net/bkerndev/Docs/isrs.htm
;   Code below, if interrupt has an error code, it is pushed to the stack before the execution of the rutine is started
;   Also, it should be poped by isr
global _isr8
global _isrA
global _isrB
global _isrC
global _isrD
global _isrE
global _isrF
global _isr10
global _isr13
global _isr20
global _isr21
global _isr80
global _isr2C

; Those with error codes SHOULD NOT push the dummy 0 error code
; List of defaultly set isrs: 8, A, B, C, D, E

;  8: Double Fault Exception (With Error Code!)
_isr8:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 8                              ; Note that we DON'T push a value on the stack in this one!
                                        ; It pushes one already! Use this type of stub for exceptions
                                        ; that pop error codes!
    jmp isr_common_stub_error_code      ; jump to common part with error code handling

;  A: Invalid TSS (With Error Code!)
_isrA:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0xA                            ; Note that we DON'T push a value on the stack in this one!
                                        ; It pushes one already! Use this type of stub for exceptions
                                        ; that pop error codes!
    jmp isr_common_stub_error_code      ; jump to common part with error code handling

;  B: Segment not present (With Error Code!)
_isrB:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0xB                            ; Note that we DON'T push a value on the stack in this one!
                                        ; It pushes one already! Use this type of stub for exceptions
                                        ; that pop error codes!
    jmp isr_common_stub_error_code      ; jump to common part with error code handling

;  C: Stack segment fault (With Error Code!)
_isrC:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0xC                            ; Note that we DON'T push a value on the stack in this one!
                                        ; It pushes one already! Use this type of stub for exceptions
                                        ; that pop error codes!
    jmp isr_common_stub_error_code      ; jump to common part with error code handling

;  D: General protection fault (With Error Code!)
_isrD:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0xD                            ; Note that we DON'T push a value on the stack in this one!
                                        ; It pushes one already! Use this type of stub for exceptions
                                        ; that pop error codes!
    jmp isr_common_stub_error_code      ; jump to common part with error code handling

;  External C function
extern _int0xE_handler

;  E: Page fault (With Error Code!)
_isrE:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    pusha                               ; Push all Registers (EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX)
    push ds                             ; Push segment registers, might be handy
    push es
    push fs
    push gs
    mov ax, 0x10                        ; Load the Kernel Data Segment descriptor! TODO: Is it ok?
    mov ds, ax                          ; Set all segment registers to Kernel
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax                          ; TODO: Is it ok???? added for 0x80, 0x20 a 0x21

    mov     eax, cr2                    ; eax = address of the page fault
    push    eax                         ; store it onto the stack
    mov eax, _int0xE_handler
    call eax                            ; A special call, preserves the 'eip' register

    pop gs
    pop fs
    pop es
    pop ds
    popa
    sti                                 ; once we're done enable interrupts
    iret

;  F: Unknown interrupt
_isrF:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0                              ; Dummy error code
    push 0xF                            ; Push interrupt code
    jmp isr_common_stub                 ; jump to common part

;  10: Coprocessor fault /  x87 Floating-Point Exception
_isr10:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0                              ; Dummy error code
    push 0x10                           ; Push interrupt code
    jmp isr_common_stub                 ; jump to common part

;  13: SIMD Floating-Point Exception
_isr13:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0                              ; Dummy error code
    push 0x13                           ; Push interrupt code
    jmp isr_common_stub                 ; jump to common part

;  20: PIT (system timer) handler
;  this handler is essential for preemptive
;  process scheduling 
_isr20:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0                              ; Dummy error code
    push 0x20                           ; Push interrupt code
    jmp isr_common_stub                 ; jump to common part

;  21: keyboard handler - it is called
;  everytime the user presses/releases a key
_isr21:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0                              ; Dummy error code
    push 0x21                           ; Push interrupt code
    jmp isr_common_stub                 ; jump to common part

;  80: system calls interrupt handler
;  this handler is used when a user process
;  wants some service from the kernel
;  (open a file, fork, exit, printf, ...)
_isr80:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0                              ; Dummy error code
    push 0x80                           ; Push interrupt code
    jmp isr_common_stub                 ; jump to common part

_isr2C:
    cli                                 ; disable interrupts (Activating another interrupt will mess up things)
    push 0                              ; Dummy error code
    push 0x2C                           ; Push interrupt code
    jmp isr_common_stub                 ; jump to common part

; We call a C function in here. We need to let the assembler know
; that '_generic_interrupt_handler' exists in another file
extern _generic_interrupt_handler

; This is our common ISR stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
isr_common_stub:
    pusha                               ; Push all Registers (EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX)
    push ds                             ; Push segment registers, might be handy
    push es
    push fs
    push gs
    mov ax, 0x10                        ; Load the Kernel Data Segment descriptor! TODO: Is it ok?
    mov ds, ax                          ; Set all segment registers to Kernel
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp                        ; Push us the stack
    push eax
    mov eax, _generic_interrupt_handler
    call eax                            ; A special call, preserves the 'eip' register
    pop eax                             ; Pop all "pushes" in reverse order
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8                          ; Cleans up the pushed error code and pushed ISR number
    sti                                 ; once we're done enable interrupts
    iret                                ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP!


; This is our common ISR stub with exceptions. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
isr_common_stub_error_code:
    pusha                               ; Push all Registers (EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX)
    push ds                             ; Push segment registers, might be handy
    push es
    push fs
    push gs
    mov ax, 0x10                        ; Load the Kernel Data Segment descriptor! TODO: Is it ok?
    mov ds, ax                          ; Set all segment registers to Kernel
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax                          ; TODO: Is it ok???? added for 0x80, 0x20 a 0x21
    mov eax, esp                        ; Push us the stack
    push eax
    mov eax, _generic_interrupt_handler
    call eax                            ; A special call, preserves the 'eip' register
    pop eax                             ; Pop all "pushes" in reverse order
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 4                          ; Cleans up the pushed ISR number
    sti                                 ; once we're done enable interrupts
    iret                                ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP!