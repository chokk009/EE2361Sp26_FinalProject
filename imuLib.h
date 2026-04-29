/* 
 * File:   imuLib.h
 * Author: aliya
 *
 * Created on April 25, 2026, 6:58 PM
 */

#ifndef IMULIB_H
#define	IMULIB_H

#ifdef	__cplusplus
extern "C" {
#endif

void i2c1_init(void); 
void imu_init(void); 
void imu_writByte(char imu_subReg, char cmd); // used to write commands to IMU
void imu_readByte(char imu_subReg); // used to read data from IMU
char imu_getByte(char imu_subReg);

int imu_getGyro_X(void);
int imu_getGyro_Y(void);
int imu_getAccel_X(void);
int imu_getAccel_Y(void);

void imu_whoAreYou(void);

#ifdef	__cplusplus
}
#endif

#endif	/* IMULIB_H */

