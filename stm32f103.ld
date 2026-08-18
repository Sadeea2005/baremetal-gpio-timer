/*
 * stm32f103.ld  --  Linker script for STM32F103C8T6 (Blue Pill).
 * Flash: 64 KB @ 0x08000000   |   SRAM: 20 KB @ 0x20000000
 *
 * The linker script is the map that tells the toolchain WHERE each section
 * lands in the chip's memory. Understanding it is a genuine firmware skill:
 * it's how you know .text/.rodata live in flash while .data/.bss live in RAM,
 * and how the startup code finds the addresses it needs to copy .data.
 */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* stack grows down from top of RAM */

SECTIONS
{
    .isr_vector :
    {
        KEEP(*(.isr_vector))           /* vector table first, at 0x08000000 */
    } > FLASH

    .text :
    {
        *(.text*)                      /* code                              */
        *(.rodata*)                    /* const data                        */
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    _sidata = LOADADDR(.data);         /* flash source of .data init values */

    .data : AT (_sidata)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
}
