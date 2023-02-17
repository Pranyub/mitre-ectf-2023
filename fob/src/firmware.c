#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "uart.h"
#include "util.h"
#include "authentication.h"

int main(void) {
    
    //init rand and uart on boot
    rand_init();
    uart_init();
    
    while(true) {
        loop();
    }
}

void loop() {
    //right now just send a hello message upon recieving any data from the host
    //in the future, send_hello() should be called when the unlock button is pressed
    //TODO: implement button read logic
    if(UARTCharsAvail(HOST_UART)) {
        UARTCharGet(HOST_UART);
        send_hello();
    }
}