/* 
 * EE2361 Spring 2026 Final Project: IMU Marble Maze
 * Group Members: Aliya Browne, Ilani Buckholdt, Anya Chokkady, Rocco Vasoli
 * 
 * File: FinalProject_main.c
 * Created on April 14, 2026, 8:52 PM
 */

// libraries and header files
#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <p24Fxxxx.h>
#include "asmLib.h"
#include "genLib.h"
#include "imuLib.h"
#include "lcdLib.h"
#include "pushLib.h"
#include "servoLib.h"

// boilerplate
// CW1: FLASH CONFIGURATION WORD 1 (see PIC24 Family Reference Manual 24.1)
#pragma config ICS = PGx1          // Comm Channel Select (Emulator EMUC1/EMUD1 pins are shared with PGC1/PGD1)
#pragma config FWDTEN = OFF        // Watchdog Timer Enable (Watchdog Timer is disabled)
#pragma config GWRP = OFF          // General Code Segment Write Protect (Writes to program memory are allowed)
#pragma config GCP = OFF           // General Code Segment Code Protect (Code protection is disabled)
#pragma config JTAGEN = OFF        // JTAG Port Enable (JTAG port is disabled)
// CW2: FLASH CONFIGURATION WORD 2 (see PIC24 Family Reference Manual 24.1)
#pragma config I2C1SEL = PRI       // I2C1 Pin Location Select (Use default SCL1/SDA1 pins)
#pragma config IOL1WAY = OFF       // IOLOCK Protection (IOLOCK may be changed via unlocking seq)
#pragma config OSCIOFNC = ON       // Primary Oscillator I/O Function (CLKO/RC15 functions as I/O pin)
#pragma config FCKSM = CSECME      // Clock Switching and Monitor (Clock switching is enabled, 
                                       // Fail-Safe Clock Monitor is enabled)
#pragma config FNOSC = FRCPLL      // Oscillator Select (Fast RC Oscillator with PLL module (FRCPLL))

// compiler constants
#define GYRO 0
#define ACCEL 1
// thresholds in Timer2 counts
#define CLICK_WINDOW 31250L  // 0.50 s
#define DEBOUNCE_WINDOW 125L        // 2ms
// push button states
#define RELEASED 1
#define PRESSED 0
// averaging sample size
#define SAMP_SIZE 50

// global variables that can be updated during runtime
int IMUmode = 0;
// global variables used in push button related ISRs
volatile unsigned long int overflow_T1 = 0; // T1
volatile unsigned long int overflow_T2 = 0; // T2, IC
volatile long int lastEventTime = 0;
volatile long int lastClickTime = 0;
volatile unsigned int prevState = RELEASED;
volatile long int pressTimes[3] = {0,0,0};
volatile int head = 0;
volatile int count = 0;
volatile int samplePrep = 0;

// ISRs
// timer 1 - global timer with period of 1ms
// timer 2 - IC1 timer with period of 1s
// timer 3 - OC timer with period of 20ms
void __attribute__((interrupt, auto_psv)) _T1Interrupt() {
    _T1IF = 0;
    samplePrep = 1;   // flag that's set to 1 every 1 ms
                      // gets set back to 0 after new imu values are read and averages are calculated
    overflow_T1++;
}
void __attribute__((interrupt, auto_psv)) _T2Interrupt() {
    _T2IF = 0;
    overflow_T2++;    // overflow variable used in timing/time difference calculations
}
void __attribute__((interrupt, auto_psv)) _IC1Interrupt() {
    unsigned int capturedTime;
    long int currentEventTime;
    long int timeDifference;
    
    _IC1IF = 0;
    
    while (IC1CONbits.ICBNE) {
        capturedTime = IC1BUF;  //read captured Timer2 value from FIFO
    
        currentEventTime = capturedTime + ((long int)(PR2 + 1) * overflow_T2);
        timeDifference = currentEventTime - lastEventTime;
    
        //Ignore bounce if edge happened too soon after previous valid edge
        if (timeDifference < DEBOUNCE_WINDOW) {
            continue;
        }
    
        //This is a valid new edge
        lastEventTime = currentEventTime;
    
        //Previous button state was released --> new state is pressed
        if (prevState == RELEASED) {
            prevState = PRESSED;
        
            // save press time in circular buffer
            pressTimes[head] = currentEventTime;
            // tracks the last time button was pressed
            lastClickTime = currentEventTime;
            head = (head + 1) % 3;
        
            if (count < 3) {
                count++;
            }
        }
    
        //Previous button state was pressed, so this valid edge is a release
        else {
            prevState = RELEASED;
        }
    }
}

// setup function
void setup(void) {
    CLKDIVbits.RCDIV = 0; // set clock frequency to 16MHz
    
    // initialize GPIOs
    AD1PCFG = 0x9fff; // set all pins to digital I/O
    TRISA = 0x0000;   // set all pins to output mode
    TRISB = 0x0000;   // set all pins to output mode 
    
    // setup T1 as a global timer with a period of 1ms
    // such that overflow = n seconds (in real time) * 1000
    T1CON = 0; // turn off timer 1 before making changes
    TMR1 = 0; // clear preexisting contents of timer 1
    T1CONbits.TCKPS = 0b00; // set T1 PRE to 1:1
    PR1 = 15999; // PR1 (reset) = (1ms)/(62.5ns*1) - 1 = 16000 - 1 = 15999
    T1CONbits.TON = 1; // turn on timer 1
    // enable T1 interrupt
    IFS0bits.T1IF = 0; // clear T1 interrupt flag
    IEC0bits.T1IE = 1; // enable T1 interrupt
    // IPC0bits.T1IP = 2; // set T1 interrupt priority

    push_init(); // initialize push button input pin and IC using T2
    imu_init(); // turn on and initialize accelerometer and gyroscope using I2C1
    initServos(); // initialize pins for servo output and OC
    initBuffers(); // initialize X and Y buffers
    lcd_init(); // initialize LCD using I2C2
}

// main function
int main(void) {
    
    setup();
    
    // startup screen
    lcd_setCursor(1,0);
    lcd_printStr("POWER ON");
    lcd_setCursor(-1,0); // hides blinking cursor offscreen
    delay_ms(1500);
    
    int XRead = 0;
    int YRead = 0;
    int old_XRead = 0;
    int old_YRead = 0;
    int avg_XRead = 0;
    int avg_YRead = 0;
    char str_XYRead[20];
    
    // State machine (begins in gyroscope mode)
    // Timing variables
    long int currentTime;
    long int doubleClickDuration;
    long int tripleClickDuration;
    int newestIndex, oldestIndex;
    
    while(1) {
        currentTime = TMR2 + ((long int)(PR2 + 1) * overflow_T2);   // computes the current time at the start of each while(1) loop. 
                                                                    
        // read IMU output buffers (AVERAGING VERSION)
        // Collects rotational acceleration data if in GYRO IMUmode and after TMR1 ISR sets ready-to-sample flag = 1
        if (IMUmode == GYRO) {
            if (samplePrep == 1) {
                // Updates buffers with new sensor readings
                putValX(imu_getGyro_X() - 1); // gyro x-output is 1 even when at rest,
                                              // so we put subtract 1 from imu_getGuro_X() before putting it into buffer
                putValY(imu_getGyro_Y());

                // Computes new averages over last (SIZE = 4) samples
                avg_XRead = getAvgX();
                avg_YRead = getAvgY();
                
                samplePrep = 0;              // resets the ready-to-sample flag 
            }
            //Updates the X and Y readings
            XRead = avg_XRead;
            YRead = avg_YRead;
        }

        //Collects linear acceleration data if in ACCEL IMUmode and after TMR1 IRS sets ready-to-sample flag = 1
        else if (IMUmode == ACCEL) {
            if (samplePrep == 1) {

                // Updates the old X and Y readings before new averages are calculated
                old_XRead = avg_XRead; 
                old_YRead = avg_YRead;

                // Updates buffers with new sensor readings
                putValX(imu_getAccel_X());
                putValY(imu_getAccel_Y());

                // Computes new averages over last (SIZE = 4) samples
                avg_XRead = getAvgX();
                avg_YRead = getAvgY();
                
                samplePrep = 0;              // resets the ready-to-sample flag
            }
            //Updates the X and Y readings
            XRead = avg_XRead;
            YRead = avg_YRead;
        }
        
//        // read IMU output buffers (NON-AVERAGING VERSION)
//        if (IMUmode == GYRO) {
//            XRead = imu_getGyro_X() - 1; // Gyro X = 1 when at rest, need to subtract 1 from reading to correct movement
//            YRead = imu_getGyro_Y();
//        }
//        else if (IMUmode == ACCEL) {
//            old_XRead = XRead; 
//            old_YRead = YRead;
//            
//            XRead = imu_getAccel_X();
//            YRead = imu_getAccel_Y();
//        }
        
        lcd_clr();
        lcd_setCursor(0,0);
        if (IMUmode == GYRO) {
            lcd_printStr("MODE:Gyro");
            Angle_move(XRead, YRead);
        }
        else if (IMUmode == ACCEL) {
            lcd_printStr("MODE:Accel");
            
            int sign_old_XRead = getSign(old_XRead);
            int sign_old_YRead = getSign(old_YRead);
            int sign_XRead = getSign(XRead);
            int sign_YRead = getSign(YRead);
            
            if (abs(sign_old_XRead) == abs(sign_XRead)) {
                if (sign_old_XRead != sign_XRead) {
                    XRead = 0;
                }
            }
            if (abs(sign_old_YRead) == abs(sign_YRead)) {
                if (sign_old_YRead != sign_YRead) {
                    YRead = 0;
                }
            }
            
            Accel_move(XRead, YRead);
        }
        
        // print current data to LCD 
        lcd_setCursor(0,1);
        sprintf(str_XYRead, "X%3d Y%3d", XRead, YRead);
        lcd_printStr(str_XYRead);
        lcd_setCursor(-1,0); // hides blinking cursor offscreen
        
        // Poll for push button input
        // Double click to block program for 2s and allow controller recalibration
        // Triple click to change between IMU gyroscope/accelerometer modes
        // Single click to move maze back to a level position
        if ((count > 0) && ((currentTime - lastClickTime) > CLICK_WINDOW)) {  // Uses currentTime from top of while(1) loop
                                                                              // Only excecutes when we have one or more button presses
                                                                              // AND (CLICK_WINDOW = 0.50 seconds) have passed between currentTime and last time button was pressed
                                                                              // When user presses button once within CLICK_WINDOW, after CLICK_WINDOW time has passed, program resisters input as single click
                                                                              // When user presses button twice within CLICK_WINDOW, after CLICK_WINDOW time has passed, program resisters input as double-click
                                                                              // When user presses button thrice within CLICK_WINDOW, after CLICK_WINDOW time has passed, program resisters input as triple-clcik
            //Triple-click push button to change imu modes
            // Check for triple-click using last 3 press times
            if (count == 3) {
                newestIndex = (head - 1 + 3) % 3;
                oldestIndex = (head - 3 + 3) % 3;

                tripleClickDuration = pressTimes[newestIndex] - pressTimes[oldestIndex];

                // If the time difference between the newest and oldest of the last three press times is
                // within 0.50 s, swicth states.
                if (tripleClickDuration <= CLICK_WINDOW) {
                    IMUmode = !IMUmode;    // Moves between IMUmode = GRYO and IMUmode = ACCEL
                    initBuffers();         // Clears all data in buffers so the buffers contain only GYRO and only ACCEL values at all times.
                }
            }
            
            //Double-click push button to pause reading from user input
            //This allows the user to reset the position of the controller to a center/level position
            //Check for double-click using last 2 press times
            // Automatically pauses when switching imu modes
            if (count >= 2) {
                newestIndex = (head - 1 + 3) % 3;
                oldestIndex = (head - 2 + 3) % 3;

                doubleClickDuration = pressTimes[newestIndex] - pressTimes[oldestIndex];

                // If the time difference between the newest and oldest of the last two press times is
                // within 0.50 s, change imu modes
                if (doubleClickDuration <= CLICK_WINDOW) {
                    lcd_setCursor(0,1);
                    lcd_printStr("          "); // clear bottom row of LCD
                    if (count == 3) {
                        lcd_setCursor(0,1);
                        lcd_printChar(0x7e);
                        lcd_setCursor(1,1);
                        lcd_printStr("Switching");
                    }
                    else if (count == 2) {
                        lcd_setCursor(2,1);
                        lcd_printStr("PAUSED");
                    }
                    lcd_setCursor(-1,0); // hides blinking cursor offscreen
                    delay_ms(2000); // blocks program for 2s to allow controller recalibration
                }
            }
            
            // If the button was only pressed once
            if(count == 1) {
                Center_tilt();    // calls function which centers the servo motors
                                  // reseting the platform back to its starting level state
            }

            // Sets the button press counter back to 0
            count = 0;  // Sets count = 0 after if ((count > 0) && ((currentTime - lastClickTime) > CLICK_WINDOW)) loop
        }
        
        delay_ms(25);
    }
    return 0;
}
