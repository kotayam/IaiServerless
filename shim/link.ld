ENTRY(_start)

SECTIONS
{
    /* Start the binary at physical address 1MB */
    . = 0x100000;

    .text : {
        *(.text.entry)  /* Force start.S to the absolute top */
        *(.text)        /* Standard code */
        *(.text.*)      /* Catch GCC's .text.startup where main() is hiding! */
    }

    .rodata : {
        *(.rodata)      /* Read-only data (like your "Hello World!" string) */
        *(.rodata.*)
    }

    .data : {
        *(.data)        /* Initialized variables */
    }

    .bss : {
        *(.bss)         /* Uninitialized variables */
    }
    
    /* Discard complex ELF metadata we don't need for a flat binary */
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
        *(.note.*)
    }
}
