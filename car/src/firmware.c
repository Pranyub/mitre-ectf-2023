#include "firmware.h"

static int random = 0;

int main(void) {
    
}

void init_entropy(void) {
    
    //need to use embedded-specific entropy in the future
    #ifdef UNIX
    random = rand();
    #endif
}