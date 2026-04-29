.include "xc.inc"          ; required "boiler-plate" (BP)
    
.text           ; BP (put the following data in ROM(program memory))

; This is a library, thus it can*not* contain a _main function;
; the C file will define main(). However, we will need a .global statement
; to make ASM functions available to C code.
; All functions utilized outside of this file will need to have a
; leading underscore (_) and be included in a comment delimited list below
.global _adc_delay1us, _adc_delay100us, _adc_delay1ms, _write_0, _write_1 
    
_adc_delay1us:	    ; 1us / 62.5ns = 16 cycles
		    ; 2 cycles for function call
    REPEAT #10	    ; 12 (repeat) + 1 (next instruction = nop) cycles
    nop
    return	    ; 3 cycles

_adc_delay100us:    ; 100us / 62.5ns = 1600 cycles
		    ; 2 cycles for function call
    REPEAT #1594    ; 1596 (repeat) + 1 (nop) cycles
    nop
    return	    ; 3 cycles
    
_adc_delay1ms:	    ; 1ms / 62.5ns = 16000 cycles
		    ; 2 cycles for function call
    REPEAT #15994   ; 15994 (repeat) + 1 (nop) cycles
    nop
    return	    ; 3 cycles
    
_write_0:
		    ; LOW for 2 cycles during function call
		
		    ; HIGH for 6 cycles total
    btg LATA, #0    ; 1 cycle - RA0 set to HIGH
    repeat #4	    ; 4 cycles
    nop		    ; 1 cycle
    
		    ; LOW for 14 cycles total (incl. function call)
    btg LATA, #0    ; 1 cycle - RA0 set to LOW
    repeat #7	    ; 7 cycles
    nop		    ; 1 cycle
    return	    ; 3 cycles
    
_write_1:
		    ; LOW for 2 cycles during function call

		    ; HIGH for 11 cycles total
    btg LATA, #0    ; 1 cycle - RA0 set to HIGH
    repeat #9	    ; 9 cycles
    nop		    ; 1 cycle
    
		    ; LOW for 9 cycles total (incl. function call)
    btg LATA, #0    ; 1 cycle - RA0 set to LOW
    repeat #2	    ; 2 cycles
    nop		    ; 1 cycle
    return	    ; 3 cycles
    