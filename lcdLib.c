#include <p24Fxxxx.h>
#include "asmLib.h"
#include "genLib.h"
#include "lcdLib.h"

const int lcd_address = 0b0111100; // 7-bit base I2C address with SA0 line pulled low (0x3C)
const int lcd_address_writ = 0b01111000; // (lcd_address << 1) & 0b11111110; // Address+R/nW(0) = 0x78
const int lcd_address_read = 0b01111001; // (lcd_address << 1) | 0b00000001; // Address+R/nW(1) = 0x79
const int lcd_contrast = 0x79;

// ISRs used in LCD operation
void __attribute__((__interrupt__, __auto_psv__)) _MI2C2Interrupt(void) {
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

    _MI2C2IF = 0; // software clear interrupt flag 
}

void i2c2_init(void) {
    // setup I2C2 for LCD
    I2C2CONbits.I2CEN = 0;
    I2C2BRG = 0x9D;   // reference: Table 16-1, FDS pg. 153
    I2C2CONbits.I2CEN = 1;
    _MI2C2IF = 0;
}
void lcd_cmd(char command) {

    //LATBbits.LATB5 = 1; // turn on heartbeat LED
    I2C2CONbits.SEN = 1; // init START condition
    while(I2C2CONbits.SEN == 1); // wait for !SEN (when START condition is complete)
    
    // address byte
    _MI2C2IF = 0;
    I2C2TRN = lcd_address_writ; // send target address and R/nW bit (0b01111000)
    // wait for interrupt flag set and status register bits clear
    //while((_MI2C2IF == 0) && (I2C2STATbits.TRSTAT == 1) && (I2C2STATbits.ACKSTAT == 1));
    while(_MI2C2IF == 0);
    
    // control byte 
        // if sending command, control byte = 0x00 (RS/DnC = 0)
        // if sending data, control byte = 0x40 (RS/DnC = 1))
    _MI2C2IF = 0;
    I2C2TRN = 0x00; // send control byte (command = 0x00)
    // wait for interrupt flag set and status register bits clear
    //while((_MI2C2IF == 0) && (I2C2STATbits.TRSTAT == 1) && (I2C2STATbits.ACKSTAT == 1));
    while(_MI2C2IF == 0);
            
    // command/data byte
    _MI2C2IF = 0;
    I2C2TRN = command; // send command byte
    // wait for interrupt flag set and status register bits clear
    //while((_MI2C2IF == 0) && (I2C2STATbits.TRSTAT == 1) && (I2C2STATbits.ACKSTAT == 1));
    while(_MI2C2IF == 0);
    
    _MI2C2IF = 0;
    I2C2CONbits.PEN = 1; // init STOP condition
    //LATBbits.LATB5 = 0; // turn off heartbeat LED
    //delay_ms(100);
    while(I2C2CONbits.PEN == 1); // wait for !PEN (when STOP condition is complete)
    
}
void lcd_config(void) {
    // general initialization process specified by LCD manufacturer
    // references: SSD pg. 61, DOGS pg. 4
    
    lcd_cmd(0x3A); // function set: RE = 1, IS = 0, REV = 0
    lcd_cmd(0x09); // extended function set (assumes RE=1): 4 line display
    lcd_cmd(0x06); // entry mode set: bottom view
    lcd_cmd(0x1E); // bias setting: BS1 = 1
    
    lcd_cmd(0x39); // function set: RE = 0; IS = 1
    lcd_cmd(0x1B); // internal OSC: BS0 = 0 -> Bias = 1/6
    lcd_cmd(0x6E); // follower control: divider on and set value
    lcd_cmd(0x56); // power control: booster on and set contrast (DB1 = C5, DB0 = C4)
    lcd_cmd(lcd_contrast); // contrast set (DB3-0 = C3-0)
    
    lcd_cmd(0x38); // function set: RE = 0, IS = 0
    lcd_cmd(0x0F); // display on, cursor on, blink on
    
    lcd_cmd(0x01); // clear display
    
}
void lcd_config_2Ldh(void) {
    // reconfigure LCD to 2-line, double height mode
    lcd_cmd(0x3A); // function set: RE = 1, IS = 0, REV = 0 
                   // DL, N, ~BE, enter extended mode RE=1, ~REV
    lcd_cmd(0x09); // extended function set (assumes RE=1)
                   // NW, ~FW, ~B/W
    lcd_cmd(0x1A); // double-height/bias/dot-shift (assumes RE=1)
                   // UD2, ~UD1, BS1, ~DH?
    lcd_cmd(0x3C); // function set: RE = 0, IS = 0
                   // DL, N, DH, return to RE=0, ~IS */
    
    lcd_cmd(0x01); // clear display
}
void lcd_init(void) {
    i2c2_init();
    // LCD start sequence specified by manufacturer
    // reset LCD after power on
    // (reset pin connected to RB14, initially pulled high due to pull-up)
    delay_ms(5);
    LATBbits.LATB14 = 0;
    delay_ms(10); // >10ms delay for hardware reset during power on sequence (SSD pg. 62)
    LATBbits.LATB14 = 1;
    delay_ms(1);

    // start LCD manufacturer's initialization process
    lcd_config();
    // re-initialize LCD to 2 line, double-height mode
    lcd_config_2Ldh();

    lcd_cmd(0x01); // clear display (one more time just in case)
}

void lcd_setCursor(char x, char y) {
    // reference: lab 5 background document pg. 13-14
    
    int cursorXY = ((0x20 & (y << 5)) | x ) | 0x80; 
    lcd_cmd(cursorXY);
}
void lcd_printChar(char myChar) {
    
    //LATBbits.LATB5 = 1; // turn on heartbeat LED
    I2C2CONbits.SEN = 1; // init START condition
    while(I2C2CONbits.SEN); // wait for !SEN (when START condition is complete)
    
    // address byte
    _MI2C2IF = 0;
    I2C2TRN = lcd_address_writ; // send target address and R/nW bit (0b01111000)
    // wait for interrupt flag set and status register bits clear
    //while((_MI2C2IF == 0) && (I2C2STATbits.TRSTAT == 1) && (I2C2STATbits.ACKSTAT == 1));
    while(I2C2STATbits.TRSTAT && !I2C2STATbits.ACKSTAT);
    
    // control byte 
        // if sending command, control byte = 0x00 (RS/DnC = 0)
        // if sending data, control byte = 0x40 (RS/DnC = 1))
    _MI2C2IF = 0;
    I2C2TRN = 0x40; // send control byte (data = 0x40)
    // wait for interrupt flag set and status register bits clear
    //while((_MI2C2IF == 0) && (I2C2STATbits.TRSTAT == 1) && (I2C2STATbits.ACKSTAT == 1));
    while(I2C2STATbits.TRSTAT && !I2C2STATbits.ACKSTAT);
            
    // command/data byte
    _MI2C2IF = 0;
    I2C2TRN = myChar; // send command byte
    // wait for interrupt flag set and status register bits clear
    //while((_MI2C2IF == 0) && (I2C2STATbits.TRSTAT == 1) && (I2C2STATbits.ACKSTAT == 1));
    while(I2C2STATbits.TRSTAT && !I2C2STATbits.ACKSTAT);
    
    _MI2C2IF = 0;
    I2C2CONbits.PEN = 1; // init STOP condition
    //LATBbits.LATB5 = 0; // turn off heartbeat LED
    //delay_ms(100);
    while(I2C2CONbits.PEN); // wait for !PEN (when STOP condition is complete)
    
}
void lcd_printStr(const char *str) {
    for (char *c = str; *c; c++) {
        lcd_printChar(*c);
    }
}
void lcd_scrollStr(const char *str) {
    char *c = str;
    
    // start printing full phrase, then scroll one letter at a time to the left
    while (*c != '\0') {
        for (char *currC = c; *currC; currC++) {
            lcd_printChar(*currC);
        }
        delay_ms(300); // scrolling speed
        lcd_cmd(0x01);
        c++;
    }
    
    // print final character
    for (char *finalC = c; *finalC; finalC++) {
        lcd_printChar(*finalC);
    }
    delay_ms(1000);
    lcd_cmd(0x01);
    
}
void lcd_clr(void) {
    lcd_cmd(0x01);
}
