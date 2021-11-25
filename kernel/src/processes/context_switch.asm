[global _switch_task]
_switch_task:
    cli

    mov ebp, [esp + 4]
    mov ecx, [ebp + 4]
    mov edx, [ebp + 8]
    mov ebx, [ebp + 12]
    mov esi, [ebp + 24]
    mov edi, [ebp + 28]

    mov ax, 0x20 | 0x03
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword 0x20 | 0x03  ; SS
    push dword [ebp + 16]   ; ESP
    push dword [ebp + 32]   ; EFLAGS
    push dword 0x18 | 0x03  ; CS
    push dword [ebp + 40]   ; EIP

    mov eax, [ebp + 0]
    mov ebp, [ebp + 20]

    iret