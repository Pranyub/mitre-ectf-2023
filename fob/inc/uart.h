#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stddef.h>
#include "util.h"

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

//send a message packet through the UART interface. uart_init() must be called first
void uart_send_message(const uint32_t PORT, Message* message);

//send raw packet through UART interface. uart_init() must be called first
void uart_send_raw(const uint32_t PORT, uint8_t* message, uint16_t size);

//initialize EEPROM
void eeprom_init(void);

//write message to an address in the eeprom
void eeprom_read(uint8_t* msg, size_t len, uint8_t* address);

//read a value from eeprom
void eeprom_write(uint8_t* msg, size_t len, uint8_t* address);


#endif