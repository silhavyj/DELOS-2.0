#include <mem/gdt.h>
#include <common.h>
#include <memory.h>

static GDT_descriptor_t gdt_desc;
static GDT_entry_t gdt_entries[GDT_ENTRY_COUNT];

static TSS_entry_t tss_entry;

static void set_gdt_entry(GDT_entry_t *entry, uint32_t base, uint32_t limit) {
    if (limit > 0xFFFFF) {
        // TODO panic (limit must not exceed 20 bits)
        _panic();
    }
    entry->base_low    = base  & 0x0000FFFF;            // bits [0 -15]
    entry->base_middle = (base  & 0x00FF0000) >> 16;    // bits [16-23] (Shifted by 16 bits to the right)
    entry->base_high   = (base  & 0xFF000000) >> 24;    // bits [24-31] (Shifted by 24 bits to the right)
    
    entry->limit_low   = limit & 0x0FFFF;               // bits [0 -15]
    entry->limit_high  = (limit & 0xF0000) >> 16;       // bits [16-19] (Shifted by 16 bits to the right)
}

extern "C" int _kernel_stack_top;

static void TSS_init() {

    memset(&tss_entry, 0, sizeof(TSS_entry_t)); //CLEAR!!!

    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(TSS_entry_t); //TODO: Is this limit size?? or limit pointer???

    // TSS (Set to E9: 1 11 0 1 0 0 1)
    set_gdt_entry(&gdt_entries[GDT_TSS_INDEX], base, limit);
    gdt_entries[GDT_TSS_INDEX].PR       = 1; // all valid segments are required to have present = 1
    gdt_entries[GDT_TSS_INDEX].PRIVL    = RING_3;
    gdt_entries[GDT_TSS_INDEX].S        = 0; // this is not a TSS segment but a data/code segment
    gdt_entries[GDT_TSS_INDEX].EX       = 1; // code segment must be executable
    gdt_entries[GDT_TSS_INDEX].DC       = 0; // can only be accessed from RING0
    gdt_entries[GDT_TSS_INDEX].RW       = 0; // readable
    gdt_entries[GDT_TSS_INDEX].AC       = 1; // access
    gdt_entries[GDT_TSS_INDEX].GR       = 1; // page granularity (4K)
    gdt_entries[GDT_TSS_INDEX].SZ       = 1; // 32 bit protected mode

    tss_entry.SS0 = KERNEL_DATA_SEG;
    tss_entry.ESP0 = (uint32_t)&_kernel_stack_top - 1;
    tss_entry.IOPB_offset = (uint16_t)sizeof(TSS_entry_t);

    tss_entry.CS = 0x0b;
    tss_entry.SS = tss_entry.DS = tss_entry.ES = tss_entry.FS = tss_entry.GS = 0x13;
}

int GDT_init() {
    // clear up all six GDT entries (the very first one is required to be NULL,
    // so it's done at this line as well)
    memset(reinterpret_cast<char *>(&gdt_entries[0]), 0, GDT_ENTRY_COUNT * sizeof(GDT_entry_t));

    // RING 0 - kernel
    set_gdt_entry(&gdt_entries[GDT_KERNEL_CODE_INDEX], 0x0, 0xFFFFF);
    gdt_entries[GDT_KERNEL_CODE_INDEX].PR       = 1; // all valid segments are required to have present = 1
    gdt_entries[GDT_KERNEL_CODE_INDEX].PRIVL    = RING_0;
    gdt_entries[GDT_KERNEL_CODE_INDEX].S        = 1; // this is not a TSS segment but a data/code segment
    gdt_entries[GDT_KERNEL_CODE_INDEX].EX       = 1; // code segment must be executable
    gdt_entries[GDT_KERNEL_CODE_INDEX].DC       = 0; // can only be accessed from RING0
    gdt_entries[GDT_KERNEL_CODE_INDEX].RW       = 1; // readable
    gdt_entries[GDT_KERNEL_CODE_INDEX].GR       = 1; // page granularity (4K)
    gdt_entries[GDT_KERNEL_CODE_INDEX].SZ       = 1; // 32 bit protected mode

    set_gdt_entry(&gdt_entries[GDT_KERNEL_DATA_INDEX], 0x0, 0xFFFFF);
    gdt_entries[GDT_KERNEL_DATA_INDEX].PR       = 1; // all valid segments are required to have present = 1
    gdt_entries[GDT_KERNEL_DATA_INDEX].PRIVL    = RING_0;
    gdt_entries[GDT_KERNEL_DATA_INDEX].S        = 1; // this is not a TSS segment but a data/code segment
    gdt_entries[GDT_KERNEL_DATA_INDEX].EX       = 0; // data segment must not be executable
    gdt_entries[GDT_KERNEL_DATA_INDEX].DC       = 0; // can only be accessed from RING0
    gdt_entries[GDT_KERNEL_DATA_INDEX].RW       = 1; // writable
    gdt_entries[GDT_KERNEL_DATA_INDEX].GR       = 1; // page granularity (4K)
    gdt_entries[GDT_KERNEL_DATA_INDEX].SZ       = 1; // 32 bit protected mode

    // RING 3 - userspace
    set_gdt_entry(&gdt_entries[GDT_USERSPACE_CODE_INDEX], 0x0, 0xFFFFF);
    gdt_entries[GDT_USERSPACE_CODE_INDEX].PR    = 1; // all valid segments are required to have present = 1
    gdt_entries[GDT_USERSPACE_CODE_INDEX].PRIVL = RING_3;
    gdt_entries[GDT_USERSPACE_CODE_INDEX].S     = 1; // this is not a TSS segment but a data/code segment
    gdt_entries[GDT_USERSPACE_CODE_INDEX].EX    = 1; // code segment must be executable
    gdt_entries[GDT_USERSPACE_CODE_INDEX].DC    = 0; // can only be accessed from any higher ring (0,1,..)
    gdt_entries[GDT_USERSPACE_CODE_INDEX].RW    = 1; // readable
    gdt_entries[GDT_USERSPACE_CODE_INDEX].GR    = 1; // page granularity (4K)
    gdt_entries[GDT_USERSPACE_CODE_INDEX].SZ    = 1; // 32 bit protected mode

    set_gdt_entry(&gdt_entries[GDT_USERSPACE_DATA_INDEX], 0x0, 0xFFFFF);
    gdt_entries[GDT_USERSPACE_DATA_INDEX].PR    = 1; // all valid segments are required to have present = 1
    gdt_entries[GDT_USERSPACE_DATA_INDEX].PRIVL = RING_3;
    gdt_entries[GDT_USERSPACE_DATA_INDEX].S     = 1; // this is not a TSS segment but a data/code segment
    gdt_entries[GDT_USERSPACE_DATA_INDEX].EX    = 0; // data segment must not be executable
    gdt_entries[GDT_USERSPACE_DATA_INDEX].DC    = 0; // can only be accessed from any higher ring (0,1,..)
    gdt_entries[GDT_USERSPACE_DATA_INDEX].RW    = 1; // writable
    gdt_entries[GDT_USERSPACE_DATA_INDEX].GR    = 1; // page granularity (4K)
    gdt_entries[GDT_USERSPACE_DATA_INDEX].SZ    = 1; // 32 bit protected mode

    // Init the Task State Segment
    TSS_init();

    // the the information about the start addr
    // of the GDT table as well as its size
    gdt_desc.size       = sizeof(gdt_entries);
    gdt_desc.start_addr = reinterpret_cast<uint32_t>(&gdt_entries[0]);

    // load the global descriptor table into the CPU
    _load_gdt(reinterpret_cast<uint32_t>(&gdt_desc));

    _tss_flush(TSS_SEG | RING_3); // TSS seg, flushed

    return 0; // everything went well
}