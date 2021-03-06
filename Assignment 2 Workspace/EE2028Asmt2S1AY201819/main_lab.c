/*****************************************************************************
 *   A demo example using several of the peripherals on the base board
 *
 *   Copyright(C) 2011, EE2024
 *   All rights reserved.
 *
 ******************************************************************************/

//#include "lpc17xx_i2c.h"
//#include "lpc17xx_ssp.h"
//#include "lpc17xx_timer.h"
//
//#include "joystick.h"
//#include "pca9532.h"
//#include "acc.h"
//#include "oled.h"
//#include "rgb.h"

#include "stdio.h"

//Step 1: for Pin config
#include "lpc17xx_pinsel.h"
//Step 2: for GPIO
#include "lpc17xx_gpio.h"
#include "LPC17xx.h" //for interrupt

void initGPIO(void){
	PINSEL_CFG_Type PinCfg;
		PinCfg.Funcnum = 0; // For GPIO

		PinCfg.OpenDrain = 0;
		PinCfg.Pinmode = 0;

		PinCfg.Portnum = 0; //port 0
		PinCfg.Pinnum = 4; // pin 4

		PINSEL_ConfigPin(&PinCfg);

		GPIO_SetDir(0, (1<<4), 0); //portNum 0, pin 4, 0 for input
}

void EINT3_IRQHandler(void){
	printf("hello\n");
	LPC_GPIOINT ->IO0IntClr = 1<<4; //clear the interrupt
}

int main(){
	//PinConfig
	initGPIO();

	//initialise the interrupt
	LPC_GPIOINT ->IO0IntEnR |= 1<<4;
	NVIC_EnableIRQ(EINT3_IRQn);

//	unit32_t GPIO_ReadValue(unit8_t portNum);
	uint32_t sw3 = 0;

	while(1){
		//For button check instead of interrupt
//		sw3 = (GPIO_ReadValue(0) >> 4) & 0x01;
//		if(sw3 == 0){
//			printf('Hi\n');
//		}
	}
}
