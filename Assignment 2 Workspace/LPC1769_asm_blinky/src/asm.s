/*
 * LPC1769_asm_blinky : asm.s
 * CK Tham, ECE, NUS
 * June 2011
 *
 * Configures PINSEL1 and GPIO to make LED2 on LPCXpresso board blink.
 */

@ Directives
		.thumb                  @ (same as saying '.code 16')
	 	.cpu cortex-m3
		.syntax unified
	 	.align 2

@ Equates
        .equ STACKINIT,		0x10008000
	    .equ PINSEL1,		0x4002C004
    	.equ FIO0DIR,		0x2009C000
    	.equ FIO0SET,		0x2009C018
    	.equ FIO0CLR,		0x2009C01C
    	.equ LEDPIN,		0x400000	@ bit 22
	  	.equ LEDDELAY,		2000000

@ Vectors
vectors:
        .word STACKINIT         @ stack pointer value when stack is empty
        .word _start + 1        @ reset vector (manually adjust to odd for thumb)
        .word _nmi_handler + 1  @
        .word _hard_fault  + 1  @
        .word _memory_fault + 1 @
        .word _bus_fault + 1    @
        .word _usage_fault + 1  @
	    .word 0            		@ checksum

		.global _start

@ Start of executable code
.section .text

_start:
	    @ set the pinselect
    	ldr r6, =PINSEL1
    	ldr r0, [r6]
    	and r0, 0xffffcfff
    	str r0, [r6]

    	@ and port direction
    	ldr r6, =FIO0DIR
    	mov r0, LEDPIN
    	str r0, [r6]

        @ Load R2 with the led pin number
        mov r2, LEDPIN         @ value to turn on/off LED

loop:
    	ldr r6, =FIO0SET
        str r2, [r6]           @ set Port 0, pin 22, turning on LED
        ldr r1, =LEDDELAY
delay1:
        subs r1, 1
        bne delay1

	    ldr r6, =FIO0CLR
        str r2, [r6]           @ clear Port 0, pin 22, turning off LED
        ldr r1, =LEDDELAY
delay2:
        subs r1, 1
        bne delay2

        b loop                 @ continue forever

@ Loop if any exception gets triggered
_exception:
_nmi_handler:
_hard_fault:
_memory_fault:
_bus_fault:
_usage_fault:
        b _exception

        .end
