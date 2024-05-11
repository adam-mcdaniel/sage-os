OUTPUT_ARCH( "riscv" )
ENTRY(_start)
MEMORY
{
  ram  (wxari) : ORIGIN = 0xFFFFFFFFF0000000, LENGTH = 4000K
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

  .text : ALIGN(4096) {
    PROVIDE(_text_start = .);
    *(.text.init) *(.text .text.*)
    PROVIDE(_text_end = .);
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
}
