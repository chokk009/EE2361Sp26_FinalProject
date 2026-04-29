/* 
 * File:   pushLib.h
 * Author: Rocco Vasoli
 *
 * Created on April 27, 2026, 10:10 AM
 */

#ifndef PUSHLIB_H
#define	PUSHLIB_H

#ifdef	__cplusplus
extern "C" {
#endif

// thresholds in Timer2 counts
#define DOUBLE_CLICK_WINDOW 31250L  // 0.50 s
#define RETURN_DELAY 125000L        // 2s
#define DEBOUNCE_WINDOW 125L        // 2ms
// push button states
#define RELEASED 1
#define PRESSED 0
    
void push_init(void);

void initBuffers(void);
void putValX(int newValue);
long getAvgX(void);
void putValY(int newValue);
long getAvgY(void);

#ifdef	__cplusplus
}
#endif

#endif	/* PUSHLIB_H */

