MEMORY
{
  /* Define each memory region */
  MFlash512 (rx) : ORIGIN = 0x0, LENGTH = 0x80000 /* 512k */
}

ENTRY(_start)

SECTIONS
{
	.text : ALIGN(4)
	{
		FILL(0xff)
		KEEP(*(.text*))
	} > MFlash512

}

