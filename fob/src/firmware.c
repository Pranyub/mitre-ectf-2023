#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "uart.h"
#include "util.h"
#include "authentication.h"

void delay(size_t counter) {
    for(size_t i = 0; i < counter; i++);
}

int main(void) {
    
    //init rand and uart on boot
    rand_init();
    uart_init();
    
    // Setup SW1
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_4MA,
                    GPIO_PIN_TYPE_STD_WPU);

    uint8_t prev_sw_state = GPIO_PIN_4;
    uint8_t curr_sw_state = GPIO_PIN_4;
    while(true) {
        
        // Poll for button press
        /******************************************************************/
        curr_sw_state = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
        if(curr_sw_state != prev_sw_state && curr_sw_state != 0) {
            delay(10000);
            if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == curr_sw_state) {
                // On button press
                start_unlock_sequence();
            } 
        }
        prev_sw_state = curr_sw_state;
        /******************************************************************/

        if(UARTCharsAvail(DEVICE_UART)) {
            parse_message();
            send_response();
        }
        
    }
}