
/* Tell the linker of the format we want */
OUTPUT_FORMAT(elf64-x86-64)

/* We want kmain as staring point*/
ENTRY(kmain)

/* Define program headers for bootloader  */
PHDRS
{
  limine_requests PT_LOAD;
  text PT_LOAD;
  rodata PT_LOAD;
  data PT_LOAD;
}

SECTIONS 
{

  /* Pick topmost 2GiB of the address space*/
  /* beginning of the region */
  . = 0xffffffff80000000;
  
  .limine_requests : {
    KEEP(* (.limine_requests_start))
    KEEP(* (.limine_requests))
    KEEP(* (.limine_requests_snd))
  } :limine_requests

  /* Move to next memory page*/
  . = ALIGN(CONSTANT(MAXPAGESIZE));

  .text : {
    *(.text .text.*)
  } :text

  /* Move to next memory page*/
  . = ALIGN(CONSTANT(MAXPAGESIZE));

  .rodata : {
    *(.rodata .rodata.*)
  } :rodata

  .note.gnu.build-id : {
      *(.note.gnu.build-id)
  } :rodata

  /* Move to next memory page for .data*/
  . = ALIGN(CONSTANT(MAXPAGESIZE));

  .data : {
    *(.data .data.*)
  } :data


  .bss : {
    *(.bss .bss.*)
    *(COMMON)
  } :data

  /DISCARD/ : {
    *(.eh_frame*)
    *(.note .note.*)
  }
}
