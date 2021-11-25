; https://wiki.osdev.org/Identity_Paging
; https://wiki.osdev.org/GRUB
; https://wiki.osdev.org/Bare_Bones
; https://wiki.osdev.org/Paging
; https://medium.com/@connorstack/how-does-a-higher-half-kernel-work-107194e46a64
;----------------------------------------------------------
;            ELSEWHERE DEFINED FUNCTIONS
;----------------------------------------------------------
extern _kernel_main                                         ; the kernel main c++ entry point
extern _c_startup                                           ; clearing up the bss section is declared elsewhere
extern _cpp_startup                                         ; calling global constructors is declared elsewhere
extern _cpp_shutdown                                        ; calling global destructors is declared elsewhere
;----------------------------------------------------------
;     CONSTANTS REGARDING THE MULTIBOOT STRUCTURE
;----------------------------------------------------------
MB_ALIGN            equ (1 << 0)                            ; align loaded modules on page boundaries
MB_MEMINFO          equ (1 << 1)                            ; provide memory map
MB_FLAGS            equ (MB_ALIGN | MB_MEMINFO)             ; this is the Multiboot 'flag' field
MB_MAGIC            equ 0x1BADB002                          ; 'magic number' lets bootloader find the header
MB_CHECKSUM         equ -(MB_MAGIC + MB_FLAGS)              ; checksum of above, to prove we are multiboot

;----------------------------------------------------------
;  TEMPORARY STORAGE FOR THE MAGIC NUMBER AND THE
;                MULTIBOOT STRUCTURE
;----------------------------------------------------------
MB_MAGIC_ADDR       equ 0x90000                             ; address of the magic number to be passed into c++ kernel (by default it's put into EAX)
MB_STRUCT_ADDR      equ 0x90004                             ; address of the multiboot structure to be passed into c++ kernel
;----------------------------------------------------------
;            CONSTANTS REGARDING PAGING
;----------------------------------------------------------
PG_DIR_ADDR         equ 0x9A000                             ; page directory address
PG_TABLE_0_ADDR     equ 0x9B000                             ; address of page table 0
PG_TABLE_1_ADDR     equ 0x9C000                             ; address of page table 1
PG_TABLE_768_ADDR   equ 0x9D000                             ; address of page table 768 (0-768 is for the userspace => 3GB)
                                                            ; 769-1023 is for the kernel => 1GB)
PG_TABLE_ENTRIES    equ 1024                                ; number of pages in one page table (4GB = 1024 tables * 1024 pages * 4096 page)
PG_PRESENT          equ (1 << 0)                            ; P (preset bit) - https://wiki.osdev.org/Paging
PG_RW               equ (1 << 1)                            ; RW (read/write bit) - https://wiki.osdev.org/Paging
PG_ATTRIBUTES       equ (PG_PRESENT | PG_RW)                ; page attributes

CR0_PG_BIT          equ (1 << 31)                           ; bit in CR0 to enable paging
CR0_PE_BIT          equ (1 << 0)                            ; bit in CR0 to enable protected mode
CR0_ENABLE_PAGING   equ (CR0_PG_BIT | CR0_PE_BIT)           ; mask applied on CR0 to enable paging
;----------------------------------------------------------
;                  MULTIBOOT SECTION
;----------------------------------------------------------
section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

;----------------------------------------------------------
;              KERNEL STACK INITIALIZATION
;----------------------------------------------------------
section .initial_stack
align 4
global _kernel_stack_bottom
_kernel_stack_bottom:
    resb    4096                                            ; the kernel stack will have 4K
global _kernel_stack_top
_kernel_stack_top:
;----------------------------------------------------------
;       TEXT SECTION - THE ENTRY POINT OF THE KERNEL
;----------------------------------------------------------
section .text
align 4
global _start                                               ; make the entry point visible for the linker
_start:
    mov     ecx, MB_MAGIC_ADDR                              ; store the magic value into its temp location, so it retrieved later on
    mov     dword [ecx], eax                                ; GRUB puts it by default into EAX but we'll need this register, hence storing it elsewhere

    mov     eax, MB_STRUCT_ADDR                             ; store the address of the multiboot structure into its temp location
    mov     dword [eax], ebx                                ; GRUB puts it by default into EBX
;----------------------------------------------------------
    mov     esp, _kernel_stack_top                          ; init the stack pointer so we can jump into C/C++
    jmp     setup_paging                                    ; jump to setup paging
;----------------------------------------------------------
;                  HIGHER-HALF KERNEL
;----------------------------------------------------------
higher_half:
    call    _c_startup                                      ; clear up the BSS section
    call    _cpp_startup                                    ; call global constructors
;----------------------------------------------------------
    mov     eax, MB_STRUCT_ADDR                             ; restore the address of the multiboot structure
    mov     ebx, dword [eax]                                ; and store it into EBX as it was before

    mov     ecx, MB_MAGIC_ADDR                              ; restore the magic number and move it
    mov     eax, dword [ecx]                                ; into EAX as it was before
;----------------------------------------------------------
    push    ebx                                             ; pass the address of the multiboot structure to the c++ kernel
    push    eax                                             ; pass the magic number to the c++ kernel
    call    _kernel_main                                    ; call the main c++ kernel entry function
;----------------------------------------------------------
    call    _cpp_shutdown                                   ; call global destructors
    cli                                                     ; disable interrupts
stop:
    hlt                                                     ; halt the system
    jmp stop                                                ; infinite loop (the program should not ever get here as it never leaves the c++ kernel)
;----------------------------------------------------------
;                 INITIALIZE PAGING
;            https://wiki.osdev.org/Paging
;----------------------------------------------------------
setup_paging:
    mov     eax, 0                                          ; index of the current page (0-1023)
    mov     ebx, PG_ATTRIBUTES                              ; page attributes
fill_table_0:                                               ; fill up the first page table (identity paging)
    mov     dword [PG_TABLE_0_ADDR + (eax * 4)], ebx        ; write the current entry
    add     ebx, 4096                                       ; move on to the next page (+4K)
    inc     eax                                             ; move on to the next entry (+4B)
    cmp     eax, 1024                                       ; if (eax == 1024) break
    je      end_0                                           ; else goto fill_table_0
    jmp     fill_table_0
end_0:
;----------------------------------------------------------
    mov     eax, 0                                          ; index of the current page (0-1023)
    mov     ebx, 0x400000 | PG_ATTRIBUTES                   ; page attributes (the page table one will map the whole 2nd MB)
fill_table_1:                                               ; fill up the first page table (map it to the 2nd MB)
    mov     dword [PG_TABLE_1_ADDR + (eax * 4)], ebx        ; write the current entry
    add     ebx, 4096                                       ; move on to the next page (+4K)
    inc     eax                                             ; move on to the next entry (+4B)
    cmp     eax, 1024                                       ; if (eax == 1024) break
    je      end_1                                           ; else goto fill_table_1
    jmp     fill_table_1
end_1:
;----------------------------------------------------------
    mov     eax, 0                                          ; index of the current page (0-1023)
    mov     ebx, PG_ATTRIBUTES                              ; page attributes
fill_table_768:                                             ; fill up the 768 page table (identity paging)
    mov     dword [PG_TABLE_768_ADDR + (eax * 4)], ebx      ; write the current entry
    add     ebx, 4096                                       ; move on to the next page (+4K)
    inc     eax                                             ; move on to the next entry (+4B)
    cmp     eax, 1024                                       ; if (eax == 1024) break
    je      end_768                                         ; else goto fill_table_768
    jmp     fill_table_768
end_768:
;----------------------------------------------------------
    mov     eax, PG_TABLE_0_ADDR                            ; store page table 0 into the page directory
    or      eax, PG_ATTRIBUTES                              ; PG_DIR[0] = PG_TABLE_0
    mov     dword [PG_DIR_ADDR], eax

    mov     eax, PG_TABLE_768_ADDR                          ; store page table 768 into the page directory
    or      eax, PG_ATTRIBUTES                              ; PG_DIR[768] = PG_TABLE_768
    mov     dword [PG_DIR_ADDR + (768 * 4)], eax

    mov     eax, PG_TABLE_1_ADDR                            ; store page table 1 into the page director
    or      eax, PG_ATTRIBUTES                              ; PG_DIR[1] = PG_TABLE_1
    mov     dword [PG_DIR_ADDR + (1 * 4)], eax
;----------------------------------------------------------
    mov     eax, PG_DIR_ADDR                                ; store the page directory addr into CR3
    mov     cr3, eax
;----------------------------------------------------------
    mov     eax, cr0                                        ; enable paging in the CR0 register
    or      eax, CR0_ENABLE_PAGING                          ; it cannot be accessed directly, hence moving it into EAX first
    mov     cr0, eax                                        ; https://en.wikipedia.org/wiki/Control_register
;----------------------------------------------------------
    lea ebx, [higher_half]                                  ; perform far jump in order to get to higher-half
    jmp ebx