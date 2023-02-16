#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "uart.h"
#include "util.h"
#include "authentication.h"

int main(void) {
    rand_init();
    uart_init();
    
    uint8_t rand[32];
    while(true) {
        if(UARTCharsAvail(HOST_UART)) {
            UARTCharGet(HOST_UART);
            send_hello();
        }
    }
}