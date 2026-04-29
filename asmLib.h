/* 
 * File:   asmLib.h
 * Author: achokkady
 *
 * Created on February 11, 2026, 2:40 PM
 */

#ifndef ASMLIB_H
#define	ASMLIB_H

#ifdef	__cplusplus
extern "C" {
#endif
    
// assembly functions
void adc_delay1us(void);
void adc_delay100us(void);
void adc_delay1ms(void);
void write_0(void);
void write_1(void);

#ifdef	__cplusplus
}
#endif

#endif	/* ASMLIB_H */

