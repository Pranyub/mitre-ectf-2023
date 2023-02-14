#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "uart.h"
#include "util.h"


int main(void) {
    uart_init();
    uint8_t* payload = "hello world!\n";
    uint8_t chall[32];
    Message m = {.size = 13, .payload = payload};
    while(true) {
    for(int i=0; i<1000000; i++) {}
    uart_send_message(HOST_UART, &m);
    }
}