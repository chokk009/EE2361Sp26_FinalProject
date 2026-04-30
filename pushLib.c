#include <p24Fxxxx.h>
#include "asmLib.h"
#include "genLib.h"
#include "pushLib.h"

// controls the size of X and Y buffers
// A higher SIZE value increases number of samples averaged
#define SIZE 4

volatile int bufferX[SIZE];
volatile int bufferY[SIZE];
volatile int buffIndexX = 0;
volatile int buffIndexY = 0;

// Function initializes push button
// sets up Timer 2 and enables an intterupt flag every time Timer 2 resets (1 second)
// and maps IC1 to RP10/RB10, capturing every volatge change on the pin
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

//Initializes both buffers
//Called when state is switched, clearing the buffers of all their values when switing between ACCEL or GRYO sensor data.
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
//Updates the X-Axis circular buffer with a new sensor reading
void putValX(int newValue) {
    bufferX[buffIndexX] = newValue;
    buffIndexX++;
    if (buffIndexX >= SIZE) {
        buffIndexX = 0;
    }
}

//When called, returns the average of the last four (SIZE) X-Axis sensor readings as a singed long
signed long getAvgX(void) {
    signed long sum = 0;
    int i;
    for (i = 0; i < SIZE; i++) {
        sum += bufferX[i];
    }
    return sum / SIZE;
}


//Y-Axis Buffer Functions
//Updates the Y-Axis circular buffer with a new sensor reading
void putValY(int newValue) {
    bufferY[buffIndexY] = newValue;
    buffIndexY++;
    if (buffIndexY >= SIZE) {
        buffIndexY = 0;
    }
}

//When called, returns the average of the last four (SIZE) Y-Axis sensor readings as a singed long
signed long getAvgY(void) {
    signed long sum = 0;
    int i;
    for (i = 0; i < SIZE; i++) {
        sum += bufferY[i];
    }
    return sum / SIZE;
}
