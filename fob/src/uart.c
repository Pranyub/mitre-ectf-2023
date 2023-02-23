#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/eeprom.h"
#include "inc/hw_memmap.h"
#include "uart.h"
#include "util.h"

/**
 * @brief Initializes device <-> device and host <-> device UART lines
 * 
 */
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


/**
 * @brief Sends a message over a given UART port
 * 
 * As of right now, the entirety of the struct is sent over.
 * Perhaps in the future we could send each field individually, but this seems to work just fine.
 * 
 * @param PORT the UART Port to use
 * @param message the message to send
 */
void uart_send_message(const uint32_t PORT, Message* message) {

    //send everything in message
    for(size_t i = 0; i < sizeof(Message); i++) {
        UARTCharPut(PORT, ((uint8_t*) message)[i]);
    }
}

/**
 * @brief Sends raw bytes over a given UART PORT
 * 
 * @param PORT the UART Port to use
 * @param message the buffer to send
 * @param size the number of bytes to send
 */
void uart_send_raw(const uint32_t PORT, void* message, uint16_t size) {
    for(size_t i = 0; i < size; i++) {
        UARTCharPut(PORT, ((uint8_t*) message)[i]);
    }
}

/**
 * @brief Attempts to read a message struct into a given buffer.
 * 
 * [NOTE]: This function offers no integrity verification!!! Corruption is definitely possible.
 * In the future, the UART buffer should be periodically flushed until a magic is found.
 * 
 * @param PORT the UART Port to use
 * @param message the buffer to write to
 * @return true if the buffer was filled up
 * @return false if the read timed out
 */
bool uart_read_message(const uint32_t PORT, Message* message) {
    size_t i = 0;
    size_t timeout = 0;
    
    #define TIMEOUT_THRESHOLD sizeof(Message) * 1000

    while(UARTCharsAvail(PORT) && i < sizeof(Message)) {

        if(timeout > TIMEOUT_THRESHOLD) {
            return false;
        }
        timeout++;
        ((uint8_t*) message)[i] = UARTCharGet(PORT);
    }

    return true;
}

/**
 * @brief Initializes the internal eeprom. Resets the board on failure.
 * 
 */
void eeprom_init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);

    //if eeprom initialization fails, reset
    if(EEPROMInit() != EEPROM_INIT_OK) {
        SysCtlReset();
    }
}

//Not sure if EEPROM is mapped as an iommu device - do some research and see if attacks that way are possible?

//TODO: Add boundry checks to read/write methods
/**
 * @brief Wrapper to read len bytes into a buffer from a given address
 * 
 * @param msg buffer to read into
 * @param len number of bytes to read
 * @param address read address (starts at 0)
 */
void eeprom_read(void* msg, size_t len, size_t address) {
    EEPROMRead(msg, address, len);
}


/**
 * @brief Wrapper to write len bytes from a buffer to a given EEPROM address
 * 
 * @param msg buffer to write from
 * @param len number of bytes to write
 * @param address write address (starts at 0)
 */
void eeprom_write(void* msg, size_t len, size_t address) {
    EEPROMProgram(msg, address, len);
}