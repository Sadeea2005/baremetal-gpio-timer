# Bare-Metal GPIO & Timer — STM32F103 (no HAL)

Firmware for an **STM32F103 "Blue Pill"** that blinks the onboard LED by writing
**peripheral registers directly** — no vendor HAL, no Arduino, no CMSIS driver
layer. A hardware timer (TIM2) paces the blink, and the project includes a
**hand-written vector table, startup code, and linker script**, so everything
that runs before `main()` is visible and understood. This is the closest small
project to real firmware: C on a microcontroller with no operating system.

## What it demonstrates

- Enabling a peripheral clock through **RCC** (nothing works until you do).
- Configuring a GPIO pin as an output via the **CRH** register.
- Driving the pin atomically with **BSRR**.
- Using a hardware **timer** (prescaler + auto-reload) as a precise time base.
- A hand-written **startup file and linker script** — the C runtime that
  initialises `.data`/`.bss` and sets the stack pointer before `main()`.

## Build

Requires the ARM bare-metal toolchain (`arm-none-eabi-gcc`) and `make`.

```bash
make          # produces blink.elf and blink.bin
```

Example output:

```
   text    data     bss     dec     hex filename
    344       0       0     344     158 blink.elf
```

`blink.bin` is the raw firmware image you flash to the chip.

> **Toolchain on Windows:** install via [MSYS2](https://www.msys2.org) in the
> UCRT64 terminal:
> `pacman -S mingw-w64-ucrt-x86_64-arm-none-eabi-gcc make`

## Run it on hardware (or a simulator)

- **Real board (STM32F103 Blue Pill):** flash with an ST-Link —
  `make flash` (needs the `stlink` tools) — or via a USB-DFU bootloader.
- **No board:** emulate an STM32F103 in [Renode](https://renode.io), or use the
  [Wokwi](https://wokwi.com) online simulator.

The onboard LED is on **PC13** and is **active-low** (writing 0 turns it ON).
On boot it blinks three times quickly (SysTick-timed), then settles into a
steady 1 Hz blink driven by TIM2.

## Design highlights

- **`volatile` register pointers** ensure every hardware read/write actually
  happens in program order and is never optimised away.
- **BSRR over ODR:** BSRR sets/clears a pin in a single atomic write, so an
  interrupt can't corrupt a read-modify-write.
- **Timer math:** `tick = clock / (PSC + 1)`. With `PSC = 7999` on the 8 MHz
  clock that's 1 kHz; `ARR = 499` overflows every 500 ms → a 1 Hz toggle.
- **Startup + linker script** copy `.data` from flash to RAM, zero `.bss`, and
  place the vector table at `0x08000000` — done by hand, not a vendor template.

## Files

```
main.c          register-level GPIO + SysTick + TIM2
startup.c       vector table, reset handler, .data/.bss init
stm32f103.ld    memory map (64 KB flash, 20 KB RAM)
Makefile        cross-compile to blink.elf / blink.bin
```

## License

Released under the MIT License — free to use and adapt.
