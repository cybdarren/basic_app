#ifndef UART_H
#define UART_H

#include <stddef.h>

/* Bare-metal SERCOM2 USART on the SAM E54 Xplained Pro.
 * PB25 = TX (SERCOM2 PAD0), PB24 = RX (SERCOM2 PAD1), routed to the
 * EDBG virtual COM port on the DEBUG USB connector. 8N1, no flow control. */

#define UART_BAUD   115200UL

#define STDOUT_FILENO_  1
#define STDERR_FILENO_  2

void uart_init(void);

/* Unix-style write(): fd 1 (stdout) and 2 (stderr) go to the UART.
 * Blocks until every byte is in the transmit register.
 * Returns the number of bytes written, or -1 for an unknown fd. */
int write(int fd, const void *buf, size_t count);

#endif /* UART_H */
