#include "asmLib.h"
#include "genLib.h"

// delay functions
void delay_ms(int delay_in_ms) {
    for (int i = 0; i < delay_in_ms; i++) {
        adc_delay1ms();
    }
}
void delay_us(int delay_in_us) {
    for (int i = 0; i < delay_in_us; i++) {
        adc_delay1us();
    }
}
int getSign(int val) {
    if (val > 0) {
        return 1;
    }
    else if (val < 0) {
        return -1;
    }
    return 0;
}