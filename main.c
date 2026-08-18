/*
 * main.c  --  Bare-metal LED blink on STM32F103 (Blue Pill), NO HAL, NO libraries.
 * ---------------------------------------------------------------------------
 * Everything here is done by writing peripheral REGISTERS directly, the way
 * HDD/firmware code does. There is no vendor HAL, no Arduino, no CMSIS driver
 * layer -- just addresses from the STM32F103 reference manual (RM0008).
 *
 * What it demonstrates:
 *   1. Enabling a peripheral clock via RCC (nothing works until you do this).
 *   2. Configuring a GPIO pin as an output through the CRH register.
 *   3. Driving the pin atomically with the BSRR register.
 *   4. Configuring a hardware TIMER (TIM2) with a prescaler + auto-reload to
 *      generate a precise time base, then polling its update flag.
 *   5. A SysTick-based millisecond delay for good measure.
 *
 * Board: STM32F103C8T6 "Blue Pill". Onboard LED is on PC13 (active-LOW:
 * writing 0 turns it ON). Default clock after reset is the 8 MHz internal HSI
 * -- we deliberately DON'T touch the PLL, to keep the timing math obvious.
 */
#include <stdint.h>

/* ---- Register definitions (base addresses from RM0008) ------------------ */
/* Each peripheral register is a 32-bit memory-mapped location. We wrap the
 * address in a volatile pointer so the compiler never caches or reorders our
 * hardware accesses. `volatile` is THE keyword embedded interviews test. */
#define REG32(addr)   (*(volatile uint32_t *)(addr))

/* Reset & Clock Control */
#define RCC_BASE      0x40021000u
#define RCC_APB2ENR   REG32(RCC_BASE + 0x18u)   /* GPIO + AFIO clock enables */
#define RCC_APB1ENR   REG32(RCC_BASE + 0x1Cu)   /* TIM2..TIM7 clock enables  */

/* GPIO port C */
#define GPIOC_BASE    0x40011000u
#define GPIOC_CRH     REG32(GPIOC_BASE + 0x04u)  /* config for pins 8..15    */
#define GPIOC_ODR     REG32(GPIOC_BASE + 0x0Cu)  /* output data register     */
#define GPIOC_BSRR    REG32(GPIOC_BASE + 0x10u)  /* atomic bit set/reset     */

/* General-purpose timer TIM2 */
#define TIM2_BASE     0x40000000u
#define TIM2_CR1      REG32(TIM2_BASE + 0x00u)   /* control register 1       */
#define TIM2_SR       REG32(TIM2_BASE + 0x10u)   /* status register          */
#define TIM2_CNT      REG32(TIM2_BASE + 0x24u)   /* counter                  */
#define TIM2_PSC      REG32(TIM2_BASE + 0x28u)   /* prescaler                */
#define TIM2_ARR      REG32(TIM2_BASE + 0x2Cu)   /* auto-reload              */

/* Cortex-M3 SysTick (core peripheral, same on every ARMv7-M part) */
#define SYSTICK_BASE  0xE000E010u
#define STK_CTRL      REG32(SYSTICK_BASE + 0x00u)
#define STK_LOAD      REG32(SYSTICK_BASE + 0x04u)
#define STK_VAL       REG32(SYSTICK_BASE + 0x08u)

#define LED_PIN       13u                        /* PC13 */

/* ---- SysTick: a simple, precise millisecond delay ----------------------- */
/* HSI = 8 MHz => 8000 core cycles per millisecond. Load 8000-1 into SysTick,
 * clock it from the core clock, and poll the COUNTFLAG (bit 16) which sets
 * each time the counter reaches zero. */
static void systick_init(void) {
    STK_LOAD = 8000u - 1u;      /* 1 ms reload at 8 MHz */
    STK_VAL  = 0u;              /* clear current value  */
    STK_CTRL = (1u << 2) |      /* CLKSOURCE = core clock */
               (1u << 0);       /* ENABLE                 */
}

static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; ++i) {
        while ((STK_CTRL & (1u << 16)) == 0u) {
            /* wait for COUNTFLAG: one millisecond elapsed */
        }
    }
}

/* ---- GPIO: configure PC13 as a 2 MHz push-pull output ------------------- */
static void led_gpio_init(void) {
    RCC_APB2ENR |= (1u << 4);          /* bit 4 = IOPCEN: enable GPIOC clock */

    /* PC13 lives in CRH. Each pin uses 4 bits: [MODE(2) | CNF(2)].
     * Pin 13 -> bits [23:20]. We want MODE=0b10 (output, 2 MHz),
     * CNF=0b00 (general-purpose push-pull) => nibble = 0b0010 = 0x2. */
    GPIOC_CRH &= ~(0xFu << 20);        /* clear the 4 config bits for pin 13 */
    GPIOC_CRH |=  (0x2u << 20);        /* set output push-pull, 2 MHz        */
}

static inline void led_on(void)  { GPIOC_BSRR = (1u << (LED_PIN + 16)); } /* reset -> LOW -> ON  */
static inline void led_off(void) { GPIOC_BSRR = (1u <<  LED_PIN);       } /* set   -> HIGH -> OFF */
static inline void led_toggle(void) { GPIOC_ODR ^= (1u << LED_PIN); }

/* ---- TIM2: a hardware time base at 2 Hz (toggles LED once per 500 ms) ---- */
/* Timer tick rate = f_clk / (PSC + 1). With PSC = 7999 and an 8 MHz clock we
 * get 1000 ticks/second. ARR = 499 makes the counter roll over (raise the
 * UIF update flag) every 500 ticks = 500 ms. We poll UIF instead of using an
 * interrupt to keep this example self-contained. */
static void tim2_init(void) {
    RCC_APB1ENR |= (1u << 0);          /* bit 0 = TIM2EN: enable TIM2 clock */
    TIM2_PSC = 7999u;                  /* 8 MHz / 8000 = 1 kHz tick         */
    TIM2_ARR = 499u;                   /* update every 500 ticks = 500 ms   */
    TIM2_CNT = 0u;
    TIM2_SR  = 0u;                     /* clear any pending flags           */
    TIM2_CR1 |= (1u << 0);             /* CEN: start the counter            */
}

static uint32_t tim2_update_elapsed(void) {
    if (TIM2_SR & (1u << 0)) {         /* UIF set? */
        TIM2_SR &= ~(1u << 0);         /* clear it (write 0) */
        return 1u;
    }
    return 0u;
}

int main(void) {
    led_gpio_init();
    systick_init();
    tim2_init();

    /* Startup signature: three quick blinks using the SysTick delay, so you
     * can see the board came alive before the steady timer-driven blink. */
    for (int i = 0; i < 3; ++i) {
        led_on();  delay_ms(80);
        led_off(); delay_ms(80);
    }

    /* Steady state: let the HARDWARE timer pace the blink -- the CPU only
     * reacts to the update flag, exactly the pattern real firmware uses so
     * the processor is free for other work. */
    for (;;) {
        if (tim2_update_elapsed()) {
            led_toggle();              /* 500 ms high, 500 ms low => 1 Hz */
        }
    }
}
