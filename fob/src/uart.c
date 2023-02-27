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
void uart_send_message(const uint32_t PORT, EncryptedMessage* message) {

    //send magic, target, and nonce
    for(size_t i = 0; i < 14 && i >= 0; i++) {
        UARTCharPut(PORT, ((uint8_t*) message)[i]);
    }

    // send the encrypted message ciphertext, but don't send the magic and target again
    for(size_t i = 2; i < sizeof(Message) && i >= 2; i++) {
        UARTCharPut(PORT, ((uint8_t*) message->msg)[i]);
    }
}

//send raw bytes over uart
void uart_send_raw(const uint32_t PORT, void* message, uint16_t size) {
    for(size_t i = 0; i < size; i++) {
        UARTCharPut(PORT, ((uint8_t*) message)[i]);
    }
}

void uart_read_message(const uint32_t PORT, EncryptedMessage* ct, Message *message) {
    size_t i = 0;
    while(UARTCharsAvail(PORT) && i < 14 && i >= 0) {
        ((uint8_t*) ct)[i] = UARTCharGet(PORT);
    }

    i = 2;
    while(UARTCharsAvail(PORT) && i < sizeof(Message) && i >= 2) {
        ((uint8_t*) message)[i] = UARTCharGet(PORT);
    }
    ct->msg = message;
}
//initialize eeprom
void eeprom_init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);

    //if eeprom initialization fails, reset
    if(EEPROMInit() != EEPROM_INIT_OK) {
        SysCtlReset();
    }
}

//Not sure if EEPROM is mapped as an iommu device - do some research and see if attacks that way are possible?

//TODO: Add boundry checks to read/write methods
//wrapper to read len bytes of a given address into a pointer (eeprom address starts at 0)
void eeprom_read(void* msg, size_t len, size_t address) {
    EEPROMRead(msg, address, len);
}


//wrapper to write len bytes of a pointer to the given eeprom address (address starts at 0)
void eeprom_write(void* msg, size_t len, size_t address) {
    EEPROMProgram(msg, address, len);
}