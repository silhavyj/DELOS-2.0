#include <boot/multiboot.h>

#include <mem/gdt.h>
#include <mem/paging.h>
#include <mem/heap.h>

#include <interrupts/irq.h>
#include <interrupts/idt.h>

#include <drivers/screen/screen.h>
#include <drivers/screen/color.h>
#include <drivers/pit/pit.h>
#include <drivers/keyboard/keyboard.h>
#include <drivers/mouse/mouse.h>

#include <fs/vfs.h>
#include <stdint.h>
#include <common.h>

#include <processes/process.h>
#include <processes/scheduler.h>

typedef void (*fn_ptr)();

extern "C" int _bss_start; // start of the bss section
extern "C" int _bss_end;   // end of the bss section

// clear up the bss section (uninitialized global variables)
extern "C" int _c_startup(void) {
    int *i;
    for (i = (int *)_bss_start; i < (int *)_bss_end; i++) {
        *i = 0;
    }
    return 0;
}

extern "C" fn_ptr __CTOR_LIST__[0]; // addr of the first constructor
extern "C" fn_ptr __CTOR_END__[0];  // addr of the last constructor

// call constructors of global declared classes
extern "C" int _cpp_startup(void) {
    fn_ptr* constructor;
    for (constructor = __CTOR_LIST__; constructor < __CTOR_END__; constructor++) {
        (*constructor)();
    }
    return 0;
}

extern "C" fn_ptr __DTOR_LIST__[0]; // addr of the first destructor
extern "C" fn_ptr __DTOR_END__[0];  // addr of the last destructor

// call destructors of global declared classes
extern "C" int _cpp_shutdown(void) {
    fn_ptr* destructor;
    for (destructor = __DTOR_LIST__; destructor < __DTOR_END__; destructor++) {
        (*destructor)();
    }
    return 0;
}

extern "C" uint32_t _kernel_virtual_start;
extern "C" uint32_t _kernel_virtual_end;

extern "C" uint32_t _kernel_physical_start;
extern "C" uint32_t _kernel_physical_end;

extern "C" uint32_t _kernel_stack_bottom;
extern "C" uint32_t _kernel_stack_top;

uint32_t kernel_size = 0;
uint32_t physical_mem_size = 0;
uint32_t kernel_stack_size = 0;

static void init_function(const char *prompt, int (*init_fce)()) {
    kprintf("%s", prompt);
    kprintf("[");
    if (init_fce() == 0) {
        set_color(FOREGROUND_GREEN);
        kprintf(" OK ");
        reset_color();
        kprintf("]\n\r");
    } else {
        set_color(FOREGROUND_RED);
        kprintf(" FAILED ");
        reset_color();
        kprintf("]\n\r");

        _panic();
    }
}

static void print_memory_info(multiboot_memory_map_t *mmap) {
    kprintf("Start addr: 0x%x | length: 0x%x | size: 0x%x | ", mmap->addr, mmap->len, mmap->size);

    // print out the type of the piece of memory
    switch (mmap->type) {
        case MULTIBOOT_MEMORY_AVAILABLE:
            kprintf("AVAILABLE\n\r");
            break;
        case MULTIBOOT_MEMORY_RESERVED:
            kprintf("RESERVED\n\r");
            break;
        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
            kprintf("ACPI_RECLAIMABLE\n\r");
            break;
        case MULTIBOOT_MEMORY_NVS:
            kprintf("NVS\n\r");
            break;
        case MULTIBOOT_MEMORY_BADRAM:
            kprintf("BADRAM\n\r");
            break;
        default: // this should not happen but just in case
            set_color(FOREGROUND_RED);
            kprintf("UNKNOWN\n\r");
            reset_color();
            break;
    }
}

// go through the memory map in order to find out how much RAM we have available
// https://wiki.osdev.org/Detecting_Memory_(x86)#Getting_a_GRUB_Memory_Map
static void scan_memory(multiboot_info_t *multibootHeader, uint8_t print_info = 0) {
    multiboot_memory_map_t *mmap;
    uint32_t mmap_addr = multibootHeader->mmap_addr;

    if (print_info)
        set_color(FOREGROUND_LIGHTGRAY);

    while (mmap_addr < multibootHeader->mmap_addr + multibootHeader->mmap_length) {
        mmap = reinterpret_cast<multiboot_memory_map_t *>(mmap_addr);

        // add the length of the current junk of memory to the total size
        physical_mem_size += mmap->len;

        if (print_info)
            print_memory_info(mmap);

        // move on the next piece of memory
        mmap_addr = mmap_addr + mmap->size + sizeof(uint32_t);
    }
    if (print_info)
        reset_color();
}

static void print_basic_kernel_info() {
    set_color(FOREGROUND_LIGHTGRAY);
    // print out how much RAM we have available
    kprintf("available RAM            : %d MB\n\r", physical_mem_size / 1024 / 1024);

    // print out the physical location of the kernel
    kprintf("kernel physical location : [0x%x   - 0x%x]\n\r", &_kernel_physical_start, &_kernel_physical_end);

    // print out the virtual location of the kernel
    kprintf("kernel virtual location  : [0x%x - 0x%x] (%d.%d MB)\n\r", &_kernel_virtual_start, &_kernel_virtual_end,
            kernel_size / 1024 / 1024, kernel_size % (1024 * 1024));

    // print out the location of the kernel stack
    kprintf("kernel stack location    : [0x%x - 0x%x] (%d KB)\n\r", &_kernel_stack_bottom, &_kernel_stack_top,
            kernel_stack_size / 1024);

    // print out location of the kernel heap
    kprintf("kernel heap location     : [0x%x - 0x%x] (%d MB)\n\r", KERNEL_HEAP_START_ADDR,
            (KERNEL_HEAP_START_ADDR + KERNEL_HEAP_SIZE), KERNEL_HEAP_SIZE / 1024 / 1024);

    // print out location of the FS
    kprintf("filesystem location      : [0x%x - 0x%x] (%d MB)\n\r", FS_START_ADDR, (FS_START_ADDR + FS_SIZE),
            FS_SIZE / 1024 / 1024);

    kprintf("free space within VFS    : %d KB\n\r", get_memory_available() / 1024);
    reset_color();
}

extern "C" int _kernel_main(uint32_t magicNumber, multiboot_info_t *multibootHeader) {
    if (magicNumber != MULTIBOOT_BOOTLOADER_MAGIC) {
        // If the magic is wrong, we should probably halt the system.
        set_color(FOREGROUND_RED);
        kprintf("Error: The OS was not compiled by a compliant bootloader!");
        _panic();
    }

    // https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Boot-information-format
    if ((multibootHeader->flags & (1 << 6)) == 0) {
        // The memory map is not present, we should probably halt the system
        set_color(FOREGROUND_RED);
        kprintf("Error: No Multiboot memory map was provided!");
        _panic();
    }

    clear_screen();  // clear the screen up
    scan_memory(multibootHeader, 1);

    // calculate the size of the kernel as well as the stack size
    kernel_size = &_kernel_physical_end - &_kernel_physical_start;
    kernel_stack_size = (uint32_t )&_kernel_stack_top - (uint32_t )&_kernel_stack_bottom;

    // initialize all modules of the OS one by one
    init_function("initializing GDT            ", &GDT_init);
    init_function("initializing PIC            ", &PIC_remap);
    init_function("initializing IDT            ", &IDT_init);
    init_function("initializing PIT            ", &PIT_init);
    init_function("initializing PS/2 keyboard  ", &keyboard_init);
    init_function("initializing PS/2 mouse     ", &mouse_init);
    init_function("initializing paging         ", &paging_init);
    init_function("initializing kernel heap    ", &kernel_heap_init);
    init_function("initializing VFS            ", &fs_init);
    init_function("initializing processes      ", &init_processes);

    print_basic_kernel_info();
    init_process_scheduler();
    switch_to_next_process();

    // there's no point of ever returning from the kernel
    while (1)
    {
    }

    _cpp_shutdown(); // call all global destructors
    return 0;
}