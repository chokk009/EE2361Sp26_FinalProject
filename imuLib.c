#include <p24Fxxxx.h>
#include "asmLib.h"
#include "genLib.h"
#include "imuLib.h"

// IMU device address (plus shifted versions)
const char imu_address = 0x6A; // 7-bit base I2C address with SA0 line pulled low = 0b1101010
const char imu_address_writ = 0xD4; // (imu_address << 1) & 0b11111110 = 0b11010100; // Address+R/nW(0)
const char imu_address_read = 0xD5; // (imu_address << 1) | 0b00000001 = 0b11010101; // Address+R/nW(1)

// global variables used in ISRs
volatile char imu_reading = 0;

// addresses of IMU internal registers which hold XL and G data readings
const char GXL = 0x22;
const char GXH = 0x23;
const char GYL = 0x24;
const char GYH = 0x25;
const char GZL = 0x26;
const char GZH = 0x27;
const char AXL = 0x28;
const char AXH = 0x29;
const char AYL = 0x2A;
const char AYH = 0x2B;
const char AZL = 0x2C;
const char AZH = 0x2D;

// ISRs used in IMU operation
void __attribute__((__interrupt__, __auto_psv__)) _MI2C1Interrupt(void) {
    // triggered by leader device events
    // from PIC24 I2C FRM pg. 18:
    // The MI2CxIF interrupt is generated on completion of the following master
    // message events:
        // ? Start condition
        // ? Stop condition
        // ? Data transfer byte transmitted or received
        // ? Acknowledge transmit
        // ? Repeated Start
        // ? Detection of a bus collision event.
    // Note: In some devices, the bus collision interrupt is not tied with the MI2CxIF interrupt.

    _MI2C1IF = 0; // software clear interrupt flag 
    
    if (I2C1STATbits.RBF) { // check receive buffer status
                            // 1 = full, 0 = not full (PIC24 I2C FRM pg. 15) 
        imu_reading = I2C1RCV; // read receive buffer value
        // RBF clears once software reads receive buffer
    }
}

void i2c1_init(void) {
    // turn off I2C1 before configuring the rest of the control register
    I2C1CONbits.I2CEN = 0;
    
    // set Baud rate generator at 100kHz (with Fcy at 16MHz)
    I2C1BRG = 0x9D;   // reference: Table 16-1, PIC24 FDS pg. 153
    
    // set ACKDT to send NACK (instead of ACK) when sending leader acknowledge
    I2C1CONbits.ACKDT = 1;
    
    // setup leader device I2C1 event interrupt
//    IFS1bits.MI2C1IF = 0; // clear interrupt flag
//    IEC1bits.MI2C1IE = 1; // enable interrupt
//    IPC4bits.MI2C1IP = 6; // set interrupt priority to second maximum
    
    // enable I2C1 module, configure SCL (RP8) and SDA (RP9) as serial port pins
    I2C1CONbits.I2CEN = 1;
    _MI2C1IF = 0;
    
}
void imu_init(void) {
    i2c1_init(); // initialize i2c and related interrupts
    
    // IMU device initialization sequence
    // turns both accelerometer and gyroscope on
    // reference: AN pages 15, 117, 123
    
    // Hardware boot sequence lasting maximum 10ms begins immediately after 
    // VDD is connected to IMU in order to load trimming parameters. 
    // IMU subregisters are inaccessible during boot sequence, so must wait 
    // a minimum of 10ms for boot sequence to finish before touching any subregisters.
    // reference: AN pg. 56
    delay_ms(15); // wait 10ms for hardware boot to finish
    
    // turn on accelerometer with ODR_XL = 52Hz, FS_XL = +/- 4g
    imu_writByte(0x10, 0x38); // write 0x38 to CTRL1_XL (0x10)
    // turn on gyroscope with ODR_G = 208Hz, FS_G = +/- 200dps
    imu_writByte(0x11, 0x5C); // write 0x5C to CTRL2_G (0x11)
    imu_writByte(0x12, 0x44); // write 0x44 to CTRL3_C (0x12)
    imu_writByte(0x13, 0x00); // write 0x00 to all other control subregisters (addresses 0x13-0x19)
    imu_writByte(0x14, 0x00);
    imu_writByte(0x15, 0x00);
    imu_writByte(0x16, 0x00);
    imu_writByte(0x17, 0x00);
    imu_writByte(0x18, 0x00);
    imu_writByte(0x19, 0x00);
    
    delay_ms(100); // wait 100ms for output to stabilize before proceeding
}
void imu_writByte(char imu_subReg, char cmd) {
    // set TRG/heartbeat LED
    //LATBbits.LATB5 = 1;
    
    // send start condition
    I2C1CONbits.SEN = 1;
    while(I2C1CONbits.SEN); // wait for !SEN (hardware clears when start condition is complete)

    // send follower device address byte (7 bit address byte + RnW = 0)
    I2C1TRN = imu_address_writ; // send target address and R/nW bit = 0 // I2C1STAT = 0x0008
    while(I2C1STATbits.TRSTAT && !I2C1STATbits.ACKSTAT); // wait for follower ACK, IF set, SR bits clear // I2C1STAT = 0x4009
    // ^ GETTING STUCK HERE 4/20 1:11pm I2C1STAT stuck at 0x4009
    
    // send follower sub-register address byte (8 bits)
    I2C1TRN = imu_subReg; // send address of IMU sub-register to be written to
    while(I2C1STATbits.TRSTAT && !I2C1STATbits.ACKSTAT); // wait for follower ACK, IF set, SR bits clear
    
    // send command byte to follower sub-register
    I2C1TRN = cmd;
    while(I2C1STATbits.TRSTAT && !I2C1STATbits.ACKSTAT); // wait for follower ack, IF set, SR bits clear

    // send stop condition
    I2C1CONbits.PEN = 1; // init STOP condition
    while(I2C1CONbits.PEN); // wait for !PEN (when STOP condition is complete)
    
     // unset TRG/heartbeat LED
    //LATBbits.LATB5 = 0;
}
void imu_readByte(char imu_subReg) { // ISR compatible
    // set TRG/heartbeat LED
    //LATBbits.LATB5 = 1;
    
    // send start condition
    _MI2C1IF = 0;
    I2C1CONbits.SEN = 1;
    while(I2C1CONbits.SEN); // wait for !SEN (hardware clears when start condition is complete)
    
    // send follower device address byte (7 bit address byte + RnW = 0)
    I2C1TRN = imu_address_writ; // send target address and R/nW bit = 0
    while(I2C1STATbits.TRSTAT && !I2C1STATbits.ACKSTAT);
    
    // send follower sub-register address byte (8 bits)
    I2C1TRN = imu_subReg; // send address of IMU sub-register to be read from
    while(I2C1STATbits.TRSTAT && !I2C1STATbits.ACKSTAT);
    
    // send repeated start
    I2C1CONbits.RSEN = 1; // init repeated start condition
    while(I2C1CONbits.RSEN); // wait for !RSEN (hardware clears when repeated start is complete)
    
    // prompt follower device to send data to leader
    I2C1TRN = imu_address_read; // send target address and R/nW bit = 1
    while(I2C1STATbits.TRSTAT && !I2C1STATbits.ACKSTAT);
    
    // receive serial memory data from follower device 
    I2C1CONbits.RCEN = 1; // enable receive mode (hardware clears after eighth bit is sampled)
    // From I2C FRM pg. 24:
        // The master can receive data from the slave device after the master has transmitted the slave
        // address with an R/W status bit value of ?1?. This is enabled by setting the RCEN bit (I2CxCON<3>
        // or I2CxCONL<3>). The master logic begins to generate clocks, and before each falling edge of
        // the SCLx, the SDAx line is sampled and data is shifted into the I2CxRSR register.
        // After the falling edge of the eighth SCLx clock, the following events occur:
            // ? The RCEN bit is automatically cleared
            // ? The contents of the I2CxRSR register transfer into the I2CxRCV register
            // ? The RBF status bit (I2CxSTAT<1>) is set
            // ? The I2C module generates the MI2CxIF interrupt
        // When the CPU reads the receive buffer (I2CxRCV), the RBF status bit is automatically cleared.
        // The user software can process the data and then execute an Acknowledge sequence.
    while(I2C1CONbits.RCEN); // wait for !RCEN
    
    // POLLING READ
    // once RCEN is unset and RBF is set, read receive buffer into software variable
    // this action should clear the RBF bit
    if (I2C1STATbits.RBF) {
        imu_reading = I2C1RCV;
    }

    // send NACK after data transmission from follower is finished
    I2C1CONbits.ACKEN = 1; // init acknowledge sequence (clears at end of sequence)
    while(I2C1CONbits.ACKEN); // wait for !ACKEN (acknowledge sequence is complete)

    // send stop condition
    I2C1CONbits.PEN = 1; // init STOP condition
    while(I2C1CONbits.PEN); // wait for !PEN (when STOP condition is complete)
    
     // unset TRG/heartbeat LED
    //LATBbits.LATB5 = 0;
}
char imu_getByte(char imu_subReg) { // polling compatible only
    char dataByte = 0;
    
    // set TRG/heartbeat LED
    //LATBbits.LATB5 = 1;
    
    // send start condition
    _MI2C1IF = 0;
    I2C1CONbits.SEN = 1;
    while(I2C1CONbits.SEN); // wait for !SEN (hardware clears when start condition is complete)
    
    // send follower device address byte (7 bit address byte + RnW = 0)
    I2C1TRN = imu_address_writ; // send target address and R/nW bit = 0
    while(I2C1STATbits.TRSTAT && !I2C1STATbits.ACKSTAT);
    
    // send follower sub-register address byte (8 bits)
    I2C1TRN = imu_subReg; // send address of IMU sub-register to be read from
    while(I2C1STATbits.TRSTAT && !I2C1STATbits.ACKSTAT);
    
    // send repeated start
    I2C1CONbits.RSEN = 1; // init repeated start condition
    while(I2C1CONbits.RSEN); // wait for !RSEN (hardware clears when repeated start is complete)
    
    // prompt follower device to send data to leader
    I2C1TRN = imu_address_read; // send target address and R/nW bit = 1
    while(I2C1STATbits.TRSTAT && !I2C1STATbits.ACKSTAT);
    
    // receive serial memory data from follower device 
    I2C1CONbits.RCEN = 1; // enable receive mode (hardware clears after eighth bit is sampled)
    // From I2C FRM pg. 24:
        // The master can receive data from the slave device after the master has transmitted the slave
        // address with an R/W status bit value of ?1?. This is enabled by setting the RCEN bit (I2CxCON<3>
        // or I2CxCONL<3>). The master logic begins to generate clocks, and before each falling edge of
        // the SCLx, the SDAx line is sampled and data is shifted into the I2CxRSR register.
        // After the falling edge of the eighth SCLx clock, the following events occur:
            // ? The RCEN bit is automatically cleared
            // ? The contents of the I2CxRSR register transfer into the I2CxRCV register
            // ? The RBF status bit (I2CxSTAT<1>) is set
            // ? The I2C module generates the MI2CxIF interrupt
        // When the CPU reads the receive buffer (I2CxRCV), the RBF status bit is automatically cleared.
        // The user software can process the data and then execute an Acknowledge sequence.
    while(I2C1CONbits.RCEN); // wait for !RCEN
    
    // POLLING READ
    // once RCEN is unset and RBF is set, read receive buffer into software variable
    // this action should clear the RBF bit
    dataByte = I2C1RCV;

    // send NACK after data transmission from follower is finished
    I2C1CONbits.ACKEN = 1; // init acknowledge sequence (clears at end of sequence)
    while(I2C1CONbits.ACKEN); // wait for !ACKEN (acknowledge sequence is complete)

    // send stop condition
    I2C1CONbits.PEN = 1; // init STOP condition
    while(I2C1CONbits.PEN); // wait for !PEN (when STOP condition is complete)
    
     // unset TRG/heartbeat LED
    //LATBbits.LATB5 = 0;
    
    return dataByte;
}

int imu_getGyro_X(void){
    char lowByte = imu_getByte(GXL);
    char highByte = imu_getByte(GXH);

    return (((highByte << 8) | lowByte))* 0.07; // dps^2
}
int imu_getGyro_Y(void){
    char lowByte = imu_getByte(GYL);
    char highByte = imu_getByte(GYH);

    return (((highByte << 8) | lowByte))* 0.07; // dps^2
}
int imu_getGyro_Z(void){
    char lowByte = imu_getByte(GZL);
    char highByte = imu_getByte(GZH);

    return (((highByte << 8) | lowByte))* 0.07; // dps^2
}
int imu_getAccel_X(void) {
    char lowByte = imu_getByte(AXL);
    char highByte = imu_getByte(AXH);

    return (((highByte << 8) | lowByte))* 0.000122 * 9.81 * 100; // cm/s^2
}
int imu_getAccel_Y(void){
    char lowByte = imu_getByte(AYL);
    char highByte = imu_getByte(AYH);

    return (((highByte << 8) | lowByte))* 0.000122 * 9.81 * 100; // cm/s^2
}
int imu_getAccel_Z(void){
    char lowByte = imu_getByte(AZL);
    char highByte = imu_getByte(AZH);

    return (((highByte << 8) | lowByte))* 0.000122 * 9.81 * 100; // cm/s^2
}

void imu_whoAreYou(void) {
    imu_readByte(0x0F);
}
