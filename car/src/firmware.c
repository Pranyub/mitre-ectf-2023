#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "uart.h"
#include "util.h"
#include "authentication.h"


int main(void) {
    // Initialization of uart, eeprom, and random functions.
    uart_init();
    rand_init();
    eeprom_init();

    //Current message
    Message current_msg;
    while(true) {
        //Wait until signal is recieved
        if(UARTCharsAvail(DEVICE_UART)) {
            //Do stuff.
            uart_read_message();
            send_next_message();
        }

    }
}