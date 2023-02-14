#ifndef UART_H
#define UART_H

#include "util.h"
#include <stdint.h>
#include <stdbool.h>

#define DEVICE_UART ((uint32_t)UART1_BASE)
#define HOST_UART ((uint32_t)UART0_BASE)

// quick fix for annoying vscode linting error

#ifndef GPIO_PB0_U1RX
#define GPIO_PB0_U1RX 0x00010001
#endif

#ifndef GPIO_PB1_U1TX
#define GPIO_PB1_U1TX 0x00010401
#endif

#ifndef GPIO_PA0_U0RX
#define GPIO_PA0_U0RX 0x00000001
#endif

#ifndef GPIO_PA1_U0TX
#define GPIO_PA1_U0TX 0x00000401
#endif

//initialize UART
void uart_init(void);

void uart_send_message(const uint32_t PORT, Message* message); //

uint8_t* create_challenge(uint8_t* entropy);

uint8_t* solve_challenge(uint8_t* entropy);

#endif