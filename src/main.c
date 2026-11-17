#include <stdint.h>
#include "definitions.h"   /* device registers + SYS_Initialize() */

/* Own names, prefixed BLINK_, so they never clash with the LED0_* macros
   MCC generates in plib_port.h when the pin has the custom name "LED0".
   (There, LED0_PIN = PORT_PIN_PC18 = 82, a global pin index, not a bit number.) */
#define CPU_HZ        120000000UL  /* must match MCC Clock Configuration */
#define BLINK_GROUP   2u           /* PORT group C = 2 (A=0, B=1, C=2) */
#define BLINK_BIT     18u          /* PC18, active low */
#define BLINK_MASK    (1UL << BLINK_BIT)

/* Busy-wait using SysTick: 1 ms per wrap, counted by COUNTFLAG. */
static void delay_ms(uint32_t ms)
{
    SysTick->LOAD = (CPU_HZ / 1000u) - 1u;
    SysTick->VAL  = 0u;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    while (ms--)
    {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0u) { }
    }
    SysTick->CTRL = 0u;
}

int main(void)
{
    /* MCC-generated chip init: wait states, clocks to 120 MHz, pins. */
    SYS_Initialize(NULL);

    /* Configure PC18 ourselves too, so the code does not depend on the pin table. */
    PORT_REGS->GROUP[BLINK_GROUP].PORT_OUTSET = BLINK_MASK;  /* LED off (active low) */
    PORT_REGS->GROUP[BLINK_GROUP].PORT_DIRSET = BLINK_MASK;  /* output */

    for (;;)
    {
        PORT_REGS->GROUP[BLINK_GROUP].PORT_OUTTGL = BLINK_MASK;  /* toggle LED0 */
        delay_ms(500u);
    }
}
