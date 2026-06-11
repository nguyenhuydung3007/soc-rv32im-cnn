/*
MEMORY {
    mem : ORIGIN = 0x00000000, LENGTH = 4k
}

SECTIONS {
    .text : {
        . = 0x00000000;
        *(.text.start)
        *(.text)
        *(.rodata)
        *(.data)
        . = ALIGN(4);
    } > mem
}
*/

MEMORY {
    ROM : ORIGIN = 0x00000000, LENGTH = 4K
    RAM : ORIGIN = 0x00010000, LENGTH = 4K
}

SECTIONS {
    .text : {
        *(.text.start)
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > ROM

    .data : {
        *(.data*)
        . = ALIGN(4);
    } > RAM

    .bss : {
        *(.bss*)
        . = ALIGN(4);
    } > RAM

    . = ALIGN(4);
    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}