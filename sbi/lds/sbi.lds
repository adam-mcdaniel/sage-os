/* sbi.lds
 * Linker script for the SBI
 * Stephen Marz
 * 19 May 2022
 * COSC562 Operating Systems: Implementation & Design
 */
/* This linker scripts exports the following symbols 
 * _memory_start - The starting memory address of the RAM.
 * _memory_end   - The ending memory address of the RAM for the SBI.
 * _text_start   - The starting memory address of the TEXT section.
 * _text_end     - The ending memory address of the TEXT section.
 * _bss_start    - The starting memory address of the BSS section.
 * _bss_end      - The ending memory address of the BSS section.
 * _data_start   - The starting memory address of the DATA section.
 * _data_end     - The ending memory address of the DATA section.
 * _rodata_start - The starting memory address of the Read-Only DATA section. 
 * _rodata_end   - The ending memory address of the Read-Only DATA section.
 * _stack_start  - The TOP of the stack area.
 * _stack_end    - The BOTTOM of the stack area. Generally, HART 0's stack will be
 *                 set to this value.
 * __global_pointer$ - Used to point to the top of the global section. Set the
 *                     GP (global pointer) register to this value.
 */
/* All text.XXYY sections are merged into a single .text section but order
 * is maintained
 */
/* Output architecture */
OUTPUT_ARCH( "riscv" )
/* The entry point to place at the top will be a global symbol _start */
ENTRY(_start)

/* Memory sections */
MEMORY
{
    /* SBI ram */
    /* (wxari):
       w - Read/Write
       x - Executable
       a - Allocatable
       i - Initialized
    */
    /* The origin of our RAM is at 0x8000_0000. The SBI
       occupies 0x8000_0000 - 0x8000_FFFF. The OS can be
       anywhere afterward since we dynamically look for the
       MAGIC number.
    */
    ram  (wxai) : ORIGIN = 0x80000000, LENGTH = 64K
}

/* Program Header Table */
PHDRS
{
    text PT_LOAD; /* Text section is placed first and loaded PT_LOAD */
    data PT_LOAD; /* Data section is placed next and loaded PT_LOAD  */
    /* We don't load the BSS section since we will be clearing it to 0 */
    bss PT_NULL;  /* BSS section is placed last, but nothing is done to the memory PT_NULL */
}

/* Determine the layout for .section XXYYZZ */
SECTIONS
{
    /* Provide two symbols _memory_start and _memory_end which we can
       import into the SBI to use */
    PROVIDE(_memory_start = ORIGIN(ram));
    PROVIDE(_memory_end = _memory_start + LENGTH(ram));

    /* Determine the layout of the text section, where CPU instructions go */
    .text : {
        PROVIDE(_text_start = .);
        /* section .text.init is loaded first. This will be placed exactly at
        0x8000_0000, where our _start symbol takes over to load the SBI */
        *(.text.init) *(.text .text.*)
        PROVIDE(_text_end = .);
    } >ram  :text  /* >region :section */

    . = ALIGN(8);
    /* We are about to start the global data sections bss, rodata, and data, so
       we need to set the __global_pointer$ to the top of this section. This symbol
       will be loaded into the gp (global pointer) register which we can use to
       derive offsets to global data */
    PROVIDE(__global_pointer$ = .);  
    .bss : {
        PROVIDE(_bss_start = .);
        /* .sbss is for short BSS data. We don't use that in assembly, but sometimes
           the C compiler will decide to load into those sections.
        */
        *(.sbss .sbss.*) *(.bss .bss.*)
        PROVIDE(_bss_end = .);
    } >ram :bss

    .rodata : {
        PROVIDE(_rodata_start = .);
        *(.rodata .rodata.*)
        PROVIDE(_rodata_end = .);
    } >ram :data

    .data : {
        /* When dealing with virtual memory, we need these to be aligned
           by a page size */        
        /* . = ALIGN(4096); */
        PROVIDE(_data_start = .);
        *(.sdata .sdata.*) *(.data .data.*)
        PROVIDE(_data_end = .);
    } >ram :data

    /* EH_HDR is typically used for DWARF2 symbols (debugging) */
    .eh_hdr : {
        *(.eh*)
    } >ram :data

    /* We set our stack to the rest of the memory we have */
    PROVIDE(_stack_start = .);
    PROVIDE(_stack_end = _memory_end);
}
