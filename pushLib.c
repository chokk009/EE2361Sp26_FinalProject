#include <p24Fxxxx.h>
#include "asmLib.h"
#include "genLib.h"
#include "pushLib.h"

#define SIZE 4

volatile int bufferX[SIZE];
volatile int bufferY[SIZE];
volatile int buffIndexX = 0;
volatile int buffIndexY = 0;

void push_init(void) {
    // push button connected to RB10
    TRISBbits.TRISB10 = 1; // set RB10 to input (TRISB10 = 1)
    CNPU2bits.CN16PUE = 1; // enable pull-up on RB10
    
    //Timer 2 setup
    T2CON = 0;
    T2CONbits.TCKPS = 0b11; // 1:256 pre-scalar
    PR2 = 62499;            // 1 second
    TMR2 = 0;
    
    _T2IE = 1;
    _T2IF = 0; // This flag goes off when the timer resets, regardless of
               // whether we use an interrupt or not
    
    //bind IC1 to RP10/RB10
    __builtin_write_OSCCONL(OSCCON & 0xBF);
    RPINR7bits.IC1R = 10;
    __builtin_write_OSCCONL(OSCCON | 0x40);
    
    IC1CON = 0;
    IC1CONbits.ICTMR = 1;
    IC1CONbits.ICM = 0b001; //captures every edge (rising and falling))
    
    _IC1IF = 0;
    _IC1IE = 1;
    
    T2CONbits.TON = 1;
}

void initBuffers(void) {
    int i;
    for (i = 0; i < SIZE; i++) {
        bufferX[i] = 0;
        bufferY[i] = 0;
    }
    buffIndexX = 0;
    buffIndexY = 0;
}

//X_axis Buffer Functions
void putValX(int newValue) {
    bufferX[buffIndexX] = newValue;
    buffIndexX++;
    if (buffIndexX >= SIZE) {
        buffIndexX = 0;
    }
}

signed long getAvgX(void) {
    signed long sum = 0;
    int i;
    for (i = 0; i < SIZE; i++) {
        sum += bufferX[i];
    }
    return sum / SIZE;
}

//Y-Axis Buffer Functions
void putValY(int newValue) {
    bufferY[buffIndexY] = newValue;
    buffIndexY++;
    if (buffIndexY >= SIZE) {
        buffIndexY = 0;
    }
}

signed long getAvgY(void) {
    signed long sum = 0;
    int i;
    for (i = 0; i < SIZE; i++) {
        sum += bufferY[i];
    }
    return sum / SIZE;
}