/* riscv.lds
 * See the SBI's linker script for an in-depth explanation
 * of each part.
 */
OUTPUT_ARCH( "riscv" )
ENTRY(_start)
MEMORY
{
  ram  (wxari) : ORIGIN = 0x80010000, LENGTH = 512M
}

PHDRS
{
  text PT_LOAD;
  data PT_LOAD;
  rodata PT_LOAD;
  bss PT_NULL;
}

SECTIONS
{
  PROVIDE(_memory_start = ORIGIN(ram));
  PROVIDE(_memory_end = _memory_start + LENGTH(ram));

  .text : {
    PROVIDE(_text_start = .);
    *(.text.init) *(.text .text.*)
    PROVIDE(_text_end = .);
  } >ram AT>ram :text

  .trampoline : ALIGN(4096) {
    . = ALIGN(4096);
    _trampoline = .;
    *(.trampoline)
    . = ALIGN(4096);
    ASSERT(. - _trampoline == 0x1000, "trampline code exceeds one page.");
  } >ram AT>ram :text

  . = ALIGN(8);
  PROVIDE(__global_pointer$ = .);

  .bss : ALIGN(4096) {
    PROVIDE(_bss_start = .);
    *(.sbss .sbss.*) *(.bss .bss.*)
    PROVIDE(_bss_end = .);
  } >ram AT>ram :bss
  
  .rodata : ALIGN(4096) {
    PROVIDE(_rodata_start = .);
    *(.rodata .rodata.*)
    PROVIDE(_rodata_end = .);
  } >ram AT>ram :rodata

  .data : ALIGN(4096) {
    . = ALIGN(4096);
    PROVIDE(_data_start = .);
    *(.sdata .sdata.*) *(.data .data.*)
    PROVIDE(_data_end = .);
  } >ram AT>ram :data

  .eh_hdr : {
    *(.eh*)
  } >ram AT>ram :data

  /* We need to make sure that the stack and heap are aligned by
   a page size, which for Risc-V (and most architectures) is 4096.
  */
  . = ALIGN(4096);

  PROVIDE(_stack_start = .);
  PROVIDE(_stack_end = _stack_start + 16K * 4);
  PROVIDE(_heap_start = _stack_end);
  PROVIDE(_heap_end = _memory_end);
}
