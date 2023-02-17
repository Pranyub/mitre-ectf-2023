#include "uart.h"
#include "inc/hw_memmap.h"
#include "util.h"

int main(void) {
    uart_init();
    uint8_t* payload = "hello world!\n";
    Message m = {1, 13, 0, 0, payload};
    while(true) {
    for(int i=0; i<1000000; i++) {}
        uart_send_message(HOST_UART, &m);
        startCar(void);
    }
}

void startCar(void){
    uint8_t* payload = "Unlocking car!";
    Message m = {2, 14, 0, 0 payload}
    for(int i=0; i <1000000; i++){}
    uart_send_message(HOST_UART, &m);
}  