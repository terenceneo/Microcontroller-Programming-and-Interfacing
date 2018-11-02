LPC1769_plainC_blinky
==========
CK Tham, ECE, NUS
June 2011

This plain C program configures GPIO Port 0 directly to make LED2 blink. 
It does not use CMSIS and Lib_MCU functions.
(This project does not define the __CODE_RED, USE_CMSIS symbols.)
 
Note:
cr_startup.c is required as it contains weak interrupt handlers and deals with ResetISR.

