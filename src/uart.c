#include <stdint.h>
#include "definitions.h"   /* device register definitions */
#include "uart.h"

/* SERCOM core clock: our own generic clock generator 3, fed from the
 * 48 MHz DFLL. (GCLK0 runs at 120 MHz, above the 100 MHz SERCOM limit.) */
#define UART_GCLK_GEN       3u
#define UART_REF_HZ         48000000UL

/* Peripheral channel index of SERCOM2 core clock in GCLK->PCHCTRL[].
 * Same value as SERCOM2_GCLK_ID_CORE in the device header. */
#define SERCOM2_PCH         23u

/* PB24 / PB25, peripheral function D = SERCOM2 */
#define UART_PORT_GROUP     1u      /* port B */
#define UART_RX_PIN         24u     /* SERCOM2 PAD1 */
#define UART_TX_PIN         25u     /* SERCOM2 PAD0 */
#define PMUX_FUNC_D         3u

/* Asynchronous arithmetic baud, 16x oversampling:
 * BAUD = 65536 * (1 - 16 * f_baud / f_ref), rounded. 63019 for 115200 @ 48 MHz. */
#define UART_BAUD_REG \
    (65536UL - (uint32_t)(((65536ULL * 16ULL * UART_BAUD) + (UART_REF_HZ / 2u)) / UART_REF_HZ))

void uart_init(void)
{
    /* 1. Generic clock generator 3 = DFLL48M / 1 = 48 MHz */
    GCLK_REGS->GCLK_GENCTRL[UART_GCLK_GEN] = GCLK_GENCTRL_SRC_DFLL
                                           | GCLK_GENCTRL_DIV(1u)
                                           | GCLK_GENCTRL_GENEN_Msk;
    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK3) != 0u) { }

    /* 2. Route generator 3 to the SERCOM2 core clock channel */
    GCLK_REGS->GCLK_PCHCTRL[SERCOM2_PCH] = GCLK_PCHCTRL_GEN(UART_GCLK_GEN)
                                         | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[SERCOM2_PCH] & GCLK_PCHCTRL_CHEN_Msk) == 0u) { }

    /* 3. Enable the SERCOM2 bus clock (APBB) */
    MCLK_REGS->MCLK_APBBMASK |= MCLK_APBBMASK_SERCOM2_Msk;

    /* 4. Pin mux: PB24 / PB25 to peripheral function D. PMUX[12] holds pins 24 (even) and 25 (odd). */
    PORT_REGS->GROUP[UART_PORT_GROUP].PORT_PINCFG[UART_RX_PIN] = PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_INEN_Msk;
    PORT_REGS->GROUP[UART_PORT_GROUP].PORT_PINCFG[UART_TX_PIN] = PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[UART_PORT_GROUP].PORT_PMUX[UART_RX_PIN / 2u] =
        PORT_PMUX_PMUXE(PMUX_FUNC_D) | PORT_PMUX_PMUXO(PMUX_FUNC_D);

    /* 5. Reset SERCOM2, then configure it as an asynchronous USART, 8N1 */
    SERCOM2_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_SWRST_Msk;
    while ((SERCOM2_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_SWRST_Msk) != 0u) { }

    SERCOM2_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK /* internal clock */
                                         | SERCOM_USART_INT_CTRLA_RXPO(1u)  /* RX on PAD1 */
                                         | SERCOM_USART_INT_CTRLA_TXPO(0u)  /* TX on PAD0 */
                                         | SERCOM_USART_INT_CTRLA_SAMPR(0u) /* 16x, arithmetic baud */
                                         | SERCOM_USART_INT_CTRLA_FORM(0u)  /* no parity */
                                         | SERCOM_USART_INT_CTRLA_DORD_Msk; /* LSB first */

    SERCOM2_REGS->USART_INT.SERCOM_BAUD = (uint16_t)UART_BAUD_REG;

    SERCOM2_REGS->USART_INT.SERCOM_CTRLB = SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT
                                         | SERCOM_USART_INT_CTRLB_SBMODE_1_BIT
                                         | SERCOM_USART_INT_CTRLB_TXEN_Msk
                                         | SERCOM_USART_INT_CTRLB_RXEN_Msk;
    while ((SERCOM2_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_CTRLB_Msk) != 0u) { }

    /* 6. Enable */
    SERCOM2_REGS->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE_Msk;
    while ((SERCOM2_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_ENABLE_Msk) != 0u) { }
}

static void uart_putc(uint8_t c)
{
    /* Wait until the data register is empty, then load the next byte */
    while ((SERCOM2_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) == 0u) { }
    SERCOM2_REGS->USART_INT.SERCOM_DATA = c;
}

int write(int fd, const void *buf, size_t count)
{
    const uint8_t *p = (const uint8_t *)buf;
    size_t i;

    if ((fd != STDOUT_FILENO_) && (fd != STDERR_FILENO_))
    {
        return -1;
    }

    for (i = 0u; i < count; i++)
    {
        uart_putc(p[i]);
    }
    return (int)count;
}
