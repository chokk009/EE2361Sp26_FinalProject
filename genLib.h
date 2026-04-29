/* 
 * File:   chokk009_genLib.h
 * Author: achokkady
 *
 * Created on April 25, 2026, 7:04 PM
 */

#ifndef GENLIB_H
#define	GENLIB_H

#ifdef	__cplusplus
extern "C" {
#endif

// C functions which call assembly functions, used in all other C libraries
void delay_ms(int delay_in_ms);
void delay_us(int delay_in_us);
int getSign(int val);

#ifdef	__cplusplus
}
#endif

#endif	/* GENLIB_H */

