#include "uart.h"
#include "util.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "inc/hw_memmap.h"

void uart_init(void) {

    //Enable Device <--> Device UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);

    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure the UART for 115,200, 8-N-1 operation.
    UARTConfigSetExpClk(
        DEVICE_UART, SysCtlClockGet(), 115200,
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
        HOST_UART, SysCtlClockGet(), 115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    
    /*********************************************************************/

    //Flush garbage data
    while (UARTCharsAvail(DEVICE_UART)) {
        UARTCharGet(DEVICE_UART);
    }
    while (UARTCharsAvail(HOST_UART)) {
        UARTCharGet(HOST_UART);
    }
}

void uart_send_message(const uint32_t PORT, Message* message) {
    /*
    UARTCharPut(PORT, message->magic);
    
    UARTCharPut(PORT, message->size & 0xff);
    UARTCharPut(PORT, (message->size >> 8) & 0xff);

    UARTCharPut(PORT, (message->c_nonce) & 0xff);
    UARTCharPut(PORT, (message->c_nonce >> 8) & 0xff);
    UARTCharPut(PORT, (message->c_nonce >> 16) & 0xff);
    UARTCharPut(PORT, (message->c_nonce >> 24) & 0xff);
    UARTCharPut(PORT, (message->c_nonce >> 32) & 0xff);
    UARTCharPut(PORT, (message->c_nonce >> 40) & 0xff);
    UARTCharPut(PORT, (message->c_nonce >> 48) & 0xff);
    UARTCharPut(PORT, (message->c_nonce >> 56) & 0xff);
    */
    for(int i = 0; i < message->size; i++) {
        UARTCharPut(PORT, ((message->payload)[i]));
    }
}

void uart_send_raw(const uint32_t PORT, uint8_t* message, uint16_t size) {
    for(int i = 0; i < size; i++) {
        UARTCharPut(PORT, message[i]);
    }
}