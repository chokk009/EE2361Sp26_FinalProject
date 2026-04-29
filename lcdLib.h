/* 
 * File:   chokk009_lcdLib.h
 * Author: achokkady
 *
 * Created on April 20, 2026, 9:03 PM
 */

#ifndef LCDLIB_H
#define	LCDLIB_H

#ifdef	__cplusplus
extern "C" {
#endif

void i2c2_init(void);
void lcd_cmd(char command);
void lcd_config(void);
void lcd_config_2Ldh(void);
void lcd_init(void); // setup lcd using i2c2 on startup

void lcd_setCursor(char x, char y);
void lcd_printChar(char myChar);
void lcd_printStr(const char *str);
void lcd_scrollStr(const char *str);
void lcd_clr(void);

#ifdef	__cplusplus
}
#endif

#endif	/* LCDLIB_H */

