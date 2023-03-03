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
    
    uint8_t first_boot_flag[10];
    eeprom_read(first_boot_flag, 10, EEPROM_FIRST_BOOT_FLAG);

    if(first_boot_flag[0] != 'F') {
        first_boot_flag[0] = 'F';
        eeprom_write("FFFFFFFFFFFF", 12, EEPROM_FIRST_BOOT_FLAG);

        uint8_t test[10];
        eeprom_read(test, 10, EEPROM_FIRST_BOOT_FLAG);

        #ifdef DEBUG
        debug_print("first boot");
        #endif
        first_boot();
    }

    secrets_init();
    
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