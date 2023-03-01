#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "uart.h"
#include "util.h"
#include "authentication.h"

#define delay(counter) \
    for(size_t i = 0; i < counter; i++);

int main(void) {
    
    //init rand and uart on boot
    uart_init();
    rand_init();
    
    
    volatile unsigned long long time_counter = 0;
    #define TIMER_THRESHOLD 1000
    
    #ifdef DEBUG
    debug_print("car start\n");
    #endif

    reset_state();

    while(true) {

        if(UARTCharsAvail(DEVICE_UART)) {

            #ifdef DEBUG
            debug_print("inc message\n");
            #endif

            if(parse_inc_message()) {
                #ifdef DEBUG
                debug_print("sending message\n");
                #endif
                send_next_message();
                time_counter = 0;
            }
        }
    }
}

void startCar(void){
    uint8_t* payload = "Unlocking car!";
    Message m = {2, 14, 0, 0 payload}
    for(int i=0; i <1000000; i++){}
    uart_send_message(HOST_UART, &m);
}  