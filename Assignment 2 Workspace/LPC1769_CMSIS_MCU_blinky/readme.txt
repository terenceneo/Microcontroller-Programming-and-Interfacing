LPC1769_CMSIS_MCU_blinky
==========
CK Tham, ECE, NUS
June 2011

(This project is adapted from LPCXpresso1768_systick.)

This project contains a LED flashing Systick example for the LPCXpresso
board mounted with an LPC1769 Cortex-M3 part.

When downloaded to the board and executed, LED2 will be illuminated.
The state of LED2 will toggle every 2 seconds, timed using the Cortex-M3's
built in Systick timer (configured using CMSIS functions).

The project makes use of code from the following projects:
- CMSISv1p30_LPC17xx : for CMSIS 1.30 files relevant to LPC17xx
- Lib_MCU : for PINSEL and GPIO functions

These libraries must exist in the same workspace in order
for the project to successfully build.

~~~~~~~~~~~~
Note that this example is only suitable for use with Red Suite / 
LPCXpresso IDE v3.6.x (v3.8.x for Linux) or later, as it makes 
use of linker related functionality introduced in this release.

More details at:
http://support.code-red-tech.com/CodeRedWiki/EnhancedManagedLinkScripts
