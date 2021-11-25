[global _panic]
_panic:
    cli                         ; diasble interrupts
panic_loop:
    hlt                         ; halt the CPU
    jmp _panic                  ; infinite loop

[global _outb]
_outb:
    mov     al, [esp + 8]       ; move the data to be sent into the al register
    mov     dx, [esp + 4]       ; move the address of the I/O port into the dx register
    out     dx, al              ; send the data to the I/O port
    ret                         ; return

[global _inb]
_inb:
    mov     dx, [esp + 4]       ; move the address of the I/O port to the dx register
    in      al, dx              ; read a byte from the I/O port and store it in the al
    ret                         ; return

[global _load_gdt]
_load_gdt:
    mov     eax, [esp + 4]      ; move GDTR address to eax
    lgdt    [eax]               ; load the address to internal segmentation register
    jmp     0x08:init_data_sp   ; perform far jump to selector 0x08 (offset into GDT, pointing at
                                ; 32bit PM code segment descriptor) to load CS with proper PM32 descriptor)
                                ; https://wiki.osdev.org/Protected_Mode
init_data_sp:
    mov     ax, 0x10            ; KERNEL_DATA_SEG
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    ret                         ; return

[global _io_wait]               ; https://wiki.osdev.org/Inline_Assembly/Examples#IO_WAIT
_io_wait:
    mov     dx, 0x80            ; 0x80 - almost unused port
    mov     ax, 0x00            ; 0x00 - data
    out     dx, al              ; send 0x00 to port 0x80
    ret                         ; return

[global _load_idt]
_load_idt:
    mov     eax, [esp + 4]      ; move IDTR address to eax
    lidt    [eax]               ; load the address
    ret                         ; return

[global _enable_interrupts]
_enable_interrupts:
    sti
    ret

[global _disable_interrupts]
_disable_interrupts:
    cli
    ret

[global _enable_paging]
_enable_paging:
    mov     eax, cr0
    or      eax, 0x80000000
    mov     cr0, eax
    ret

[global _disable_paging]
_disable_paging:
    mov     eax, cr0
    and     eax, 0x7FFFFFFF
    mov     cr0, eax
    ret

[global _load_page_dir]
_load_page_dir:
    mov     eax, [esp + 4]
    mov     cr3, eax
    ret

[global _get_page_dir]
_get_page_dir:
    mov     eax, cr3
    ret

[global _flush_tlb]
_flush_tlb:
    mov     eax, [esp + 4]
    invlpg  [eax]
    ret

[global _tss_flush]
_tss_flush:
    mov eax, [esp + 4]  ; Load the index of our TSS structure - The index is
                        ; 0x28, as it is the 5th selector and each is 8 bytes
                        ; long, but we set the bottom two bits (making 0x2B)
                        ; so that it has an RPL of 3, not zero.
    ltr ax              ; Load 0x2B into the task state register.
    ret