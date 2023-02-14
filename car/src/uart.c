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
    for(int i = 0; i < MESSAGE_HEADER_SIZE; i++) {
        UARTCharPut(PORT, (((char*)message)[i]));
    }

    for(int i = 0; i < message->size; i++) {
        UARTCharPut(PORT, (((char*)message->payload)[i]));
        UARTCHarPut(PORT, (create_challenge((char*)message->payload)[i]));
        UARTCHarPut(PORT, (solve_challenge((char*)message->payload)[i]));
    }
}

//Using ceaser cipher for now:
uint8_t* create_challenge(uint8_t* payload){
    for (int i = 0; payload[i] != '\0'; ++i) {

        char ch = payload[i];
        uint8_t lower = ch - 'a';
        uint8_t upper = ch - 'A';

        // lower case characters
        if (lower < 26 && lower >= 0) {
            ch = (ch - 'a' + 3) % 26 + 'a';
        }
        // uppercase characters
        if (upper < 26 && upper >= 0) {
            ch = (ch - 'A' + 3) % 26 + 'A';
        }
        payload[i] = ch;
    }
    return payload;
}

//Using ceaser cipher for now:
uint8_t* solve_challenge(uint8_t* payload){
    for (int i = 0; payload[i] != '\0'; ++i) {

        char ch = payload[i];
        uint8_t lower = ch - 'a';
        uint8_t upper = ch - 'A';

        // lower case characters
        if (lower < 26 && lower >= 0) {
            ch = (ch - 'a' - 3) % 26 + 'a';
        }
        // uppercase characters
        if (upper < 26 && upper >= 0) {
            ch = (ch - 'A' - 3) % 26 + 'A';
        }
        payload[i] = ch;
    }
    return payload;
}