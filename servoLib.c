#include "xc.h"
#include "servoLib.h"

//Original scales created by finding slope when PWM was y-axis and Acceleration/Angles were x-axis.
//New Scales made by testing, since originals made movements jerky.

#define AngleScale (1000.0f/500.0f) //Scales angles to PWM
#define AccelScale (1000.0f/1500.0f)    //Scales acceleration to PWM

//Global variables
double X = 3000;     //X PWM initialized at center
double Y = 3000;     //Y PWM initialized at center
   
void initServos(void){      //initializes servo motors
    _RCDIV = 0; 
    AD1PCFG = 0xffff; //all digital
    
    //Sets the Timer 3 registers to have a period of 20ms
    T3CONbits.TCKPS = 1;   // 1:8 prescaler
    PR3 = 39999;           // 20 ms period
    TMR3 = 0;              // reset timer
    T3CONbits.TON = 1;     // start Timer 3
    
    
  //Uses PPS to bind Output Compare 1 to RP6.  SERVO 1
    TRISBbits.TRISB6 = 0;
    __builtin_write_OSCCONL(OSCCON & 0xBF); //unlock
    RPOR3bits.RP6R = 18; //RP6/RB6
    __builtin_write_OSCCONL(OSCCON | 0x40); //lock
   
    OC1CON = 0;    // turn off OC1 for now
    OC1R = 3000;   // servo start position. We won't touch OC1R again
    OC1RS = 3000;  // We will only change this once PWM is turned on
    OC1CONbits.OCTSEL = 1; // Use Timer 3 for compare source
    OC1CONbits.OCM = 0b110; // Output compare PWM w/o faults

    
    //Uses PPS to bind Output Compare 2 to RP7.  SERVO 2
    TRISBbits.TRISB7 = 0;
    __builtin_write_OSCCONL(OSCCON & 0xBF); //unlock
    RPOR3bits.RP7R = 19;  //RP7/RB7
     __builtin_write_OSCCONL(OSCCON | 0x40); //lock
   
    OC2CON = 0;    // turn off OC2 for now
    OC2R = 3000;   // servo start position. We won't touch OC2R again
    OC2RS = 3000;  // We will only change this once PWM is turned on
    OC2CONbits.OCTSEL = 1; // Use Timer 3 for compare source
    OC2CONbits.OCM = 0b110; // Output compare PWM w/o faults
}

void setServo1(int Val){  //Moves Servo 1
    OC1RS = Val;
}

void setServo2(int val){  //Moves Servo 2
    OC2RS = val;
}

void Move(double X_PWM, double Y_PWM){  //Moves Servos to desired PWM within safe values for both Servos and avoiding the support column.
    
    if(X_PWM<=4000 && X_PWM>=2195 ) {  //Angles are within safe PWM
        setServo1(X_PWM);
    }
    else if(X_PWM >4000) { //If angles are not safe, the closest safe angle is selected
        setServo1(4000);
        X = 4000;
    }
    else if(X_PWM <2195) {
        setServo1(2195);
        X = 2195;
    }
    //delay_ms(50);
     
    if(Y_PWM<=4000 && Y_PWM>=2195 ) { //Bounds are tighter for Y because of the support column
        setServo2(Y_PWM);
    }
    else if(Y_PWM >4000) {
        setServo2(4000);
        Y = 4000;
    }
    else if(Y_PWM <2195) {
        setServo2(2195);
        Y = 2000;
    }
    //delay_ms(50);
}

void Angle_move(double dy, double dx){          //Moves servos to previous state + change, limiting motion to safe PWM

   //negative signs allow the table to go in the same direction as the gyroscope tilts.
    double Xangle_to_PWM = (AngleScale*dx)+X;    //PWM of the users current X angle
    double Yangle_to_PWM = -(AngleScale*dy)+Y;   //PWM of the users current Y angle
    
    X = Xangle_to_PWM;  //True angles continue updating no matter what.
    Y = Yangle_to_PWM;
    
    Move(Xangle_to_PWM, Yangle_to_PWM);
}

void Accel_move(double ax, double ay){         //similar to Angle_move but for accelerations
    double Xaccel_to_PWM = -AccelScale*ax+X;    //PWM of the users X acceleration
    double Yaccel_to_PWM = -AccelScale*ay+Y;   //PWM of the users Y acceleration
    
    X = Xaccel_to_PWM;  //True angles continue updating no matter what.
    Y = Yaccel_to_PWM;
    
    Move(Xaccel_to_PWM, Yaccel_to_PWM);
}

void Center_tilt(void){ //Moves Table to flat position.
    setServo1(3000);
    setServo2(3000);
    X=3000;
    Y=3000;
}
