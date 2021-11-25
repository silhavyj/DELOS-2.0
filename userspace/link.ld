ENTRY(_start)

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
    . = 0x800000;

    .text ALIGN (0x1000) : AT(ADDR(.text))
    {
        *(.multiboot)
        *(.text*)
    }

    .rodata ALIGN (0x1000) : AT(ADDR(.rodata))
    {
        *(.rodata)
    }

    .data ALIGN (0x1000) : AT(ADDR(.data))
    {
        __CTOR_LIST__ = .; *(.ctors) *(.init_array) __CTOR_END__ = .;
        __DTOR_LIST__ = .; *(.dtors) *(.fini_array) __DTOR_END__ = .;
        data = .;
        _data = .;
        __data = .;
        *(.data)
    }

    __bss_start = .;
    .bss ALIGN (0x1000) : AT(ADDR(.bss))
    {
        *(COMMON)
        *(.bss)
    }
    __bss_end = .;
}