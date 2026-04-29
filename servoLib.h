/* 
 * File:   servoLib.h
 * Author: ilani
 *
 * Created on April 27, 2026, 3:02 PM
 */

#ifndef SERVOLIB_H
#define	SERVOLIB_H

#ifdef	__cplusplus
extern "C" {
#endif
   
   void initServos(void);
   void setServo1(int Val);
   void setServo2(int val);
   void Move(double X_PWM, double Y_PWM);
   void Angle_move(double dx, double dy);
   void Accel_move(double ax, double ay);
   void Center_tilt(void);


#ifdef	__cplusplus
}
#endif

#endif	/* SERVOLIB_H */
