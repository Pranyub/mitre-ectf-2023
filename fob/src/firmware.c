#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "uart.h"
#include "util.h"
#include "authentication.h"

int main(void) {
    rand_init();
    uart_init();
    
    uint8_t rand[32];
    while(true) {
    for(int i=0; i<5000000; i++) {}
    rand_get_bytes(rand, 32);
    uart_send_raw(HOST_UART, rand, 32);
    }
}