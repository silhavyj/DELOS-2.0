ENTRY(_start)

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

KERNEL_VIRTUAL_BASE = 0xC0000000;

SECTIONS
{
    . = 0x00100000;
    . += KERNEL_VIRTUAL_BASE;

    _kernel_virtual_start = .;
    _kernel_physical_start = . - KERNEL_VIRTUAL_BASE;

    .text ALIGN (0x1000) : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE)
    {
        *(.multiboot)
        *(.text*)
    }

    .rodata ALIGN (0x1000) : AT(ADDR(.rodata) - KERNEL_VIRTUAL_BASE)
    {
        *(.rodata)
    }

    .data ALIGN (0x1000) : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE)
    {
        __CTOR_LIST__ = .; *(.ctors) *(.init_array) __CTOR_END__ = .;
        __DTOR_LIST__ = .; *(.dtors) *(.fini_array) __DTOR_END__ = .;
        data = .;
        _data = .;
        __data = .;
        *(.data)
    }

    _bss_start = .;
    .bss ALIGN (0x1000) : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE)
    {
        *(COMMON)
        *(.bss)
    }
    _bss_end = .;

    _kernel_virtual_end = .;
    _kernel_physical_end = . - KERNEL_VIRTUAL_BASE;
}