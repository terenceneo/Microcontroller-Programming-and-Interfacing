/*
 * LPC1769_plainC_blinky
 * ==========
 * CK Tham, ECE, NUS
 * June 2011
 *
 * This plain C program configures GPIO Port 0 directly to make LED2 blink.
 * It does not use CMSIS and Lib_MCU functions.
 */

#define LEDPIN  (1 << 22)
#define FIO0DIR 0x2009C000
#define FIO0PIN 0x2009C014

int main(void) {

	volatile unsigned int i;

	volatile unsigned int *p = (unsigned int *)FIO0DIR;
	*p = LEDPIN;

	while(1)
	{
		p = (unsigned int *)FIO0PIN;
		if (*p & LEDPIN)
			p += 2;
		else
			p++;
		*p = LEDPIN;
		for (i = 0; i < 1000000; i++);
	}
	return 0;
}
