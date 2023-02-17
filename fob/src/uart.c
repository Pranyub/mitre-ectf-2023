#include <stdint.h>
#include <stddef.h>

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/eeprom.h"
#include "inc/hw_memmap.h"
#include "uart.h"
#include "util.h"

void uart_init(void) {

    //Enable Device <--> Device UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);

    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure the UART for 115,200, 8-N-1 operation.
    UARTConfigSetExpClk(
        (uint32_t)UART1_BASE, SysCtlClockGet(), 115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    /*********************************************************************/

    //Enable Computer <--> Device UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure the UART for 115,200, 8-N-1 operation.
    UARTConfigSetExpClk(
        (uint32_t)UART0_BASE, SysCtlClockGet(), 115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    
    /*********************************************************************/

    //Flush garbage data
    while (UARTCharsAvail((uint32_t)UART1_BASE)) {
        UARTCharGet((uint32_t)UART1_BASE);
    }
    while (UARTCharsAvail((uint32_t)UART0_BASE)) {
        UARTCharGet((uint32_t)UART0_BASE);
    }
}


// send a message packet over uart
void uart_send_message(const uint32_t PORT, Message* message) {
    for(uint8_t i = 0; i < sizeof(Message) - sizeof(void*); i++) {
        UARTCharPut(PORT, ((uint8_t*)message)[i]);
    }

    //!!! what if message->payload_size is corrupted?
    for(uint8_t i = 0; i < message->payload_size; i++) {
        UARTCharPut(PORT, ((uint8_t*)message->payload)[i]);
    }

}

//send raw bytes over uart
void uart_send_raw(const uint32_t PORT, uint8_t* message, uint16_t size) {
    for(int i = 0; i < size; i++) {
        UARTCharPut(PORT, message[i]);
    }
}

//initialize eeprom
void eeprom_init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);

    //if eeprom initialization fails, reset
    if(EEPROMInit() != EEPROM_INIT_OK) {
        SysCtlReset();
    }
}

//TODO: Add boundry checks to read/write methods
void eeprom_read(uint8_t* msg, size_t len, size_t address) {
    EEPROMRead(msg, address, len);
}

void eeprom_write(uint8_t* msg, size_t len, size_t address) {
    EEPROMProgram(msg, address, len);
}