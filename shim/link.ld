ENTRY(_start)

SECTIONS
{
    /* Start the binary at physical address 1MB (0x100000) */
    . = 0x100000;

    /* --- CODE SEGMENT (Read / Execute) --- */
    .text : {
        *(.text.entry)
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    /* * Push the next section to the next 4KB memory page boundary.
     * This forces the linker to create a new PT_LOAD segment!
     */
    . = ALIGN(0x1000);

    /* --- DATA SEGMENT (Read / Write) --- */
    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
    
    _end = .;
    
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
        *(.note.*)
    }
}
