/*****************************************************************************
 *   A demo example using several of the peripherals on the base board
 *
 *   Copyright(C) 2011, EE2024
 *   All rights reserved.
 *
 ******************************************************************************/

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"

#include "joystick.h"
#include "pca9532.h"
#include "acc.h"
#include "oled.h"
#include "rgb.h"
#include "led7seg.h"
#include "light.h"

#include "stdio.h"
#include "math.h"
#include "stdlib.h"

#include "lpc17xx_uart.h"
#include "uart2.h"

#define NOTE_PIN_HIGH() GPIO_SetValue(0, 1<<26);
#define NOTE_PIN_LOW()  GPIO_ClearValue(0, 1<<26);

 uint32_t msTicks;

int CLEAR_WARNING = 1; //SW4
volatile int MODE_TOGGLE = 0; //Read SW3 with interrupt
int MODE_CLEAR; //Poll signal from SW4
volatile int INT_COUNT = 0;//Interrupt count

int ACC_WARNING = 0;
float ACC_THRESHOLD = 0.4;

int TEMP_WARNING = 0;
float TEMP_HIGH_THRESHOLD = 33;

int OBST_WARNING = 0 ;
float OBST_NEAR_THRESHOLD = 3000;
float MAX_LIGHT_INTENSITY = 3892;
float MAX_THRESHOLD_DIFF = 892;

int RGB = 1;
int clear_screen = 0;
volatile uint32_t t2;
volatile uint32_t t1;
volatile int j = 0;
float TEMP;

volatile int error_message=0;
volatile int error_message_TA=0;
static char* msg = NULL;

uint32_t ini_t_TA;
int8_t x = 0;
int8_t y = 0;
int8_t z = 0;
int8_t xoff = 4;
int8_t yoff = -20;
uint32_t sound_flag = 0;
uint32_t sound_toggle = 0;
uint8_t data = 0;
int count_log_in = 0;
int log_in = 0;
int INT_COUNT_UART3 = 0;
int mode = 1;
int countdown;
int i = 15;
uint8_t charaters[16] ={0x24,0x7D,0xE0,0x70,0x39,0x32,0x22,0x7C,0x20,0x30,0x28,0x23,0xA6,0x61,0xA2,0xAA};
uint32_t init_t;
uint32_t PO_init_t;
uint32_t ini_t_launch;
uint32_t ini_t_launch_2;
int getTick_once = 0;
int buzzer_count = 0;
int MODE_TOGGLE_Count = 0;
int stat_mess_count = 0;
float x_g;
float y_g;
char display_acc[80];

void SysTick_Handler(void)
{
	msTicks++;
	if(TEMP_WARNING==1 || ACC_WARNING==1 && CLEAR_WARNING != 0 && mode != 3 || OBST_WARNING == 1){
	if (sound_flag >= 0 && sound_flag <= 500)
	{
		if (sound_toggle ==2)
		{
			GPIO_SetValue(0, 1<<26);
			sound_toggle = 0;
		}
		else
		{
			GPIO_ClearValue(0, 1<<26);
			sound_toggle++;
		}
	}
	else if (sound_flag >= 1000)
	{
		sound_flag = 0;
	}
	sound_flag++;
}
}

uint32_t getTicks(void){
	return msTicks;
}


static void playNote(uint32_t note, uint32_t durationMs) {

    uint32_t t = 0;

    if(note>0) {

        while (t < (durationMs*1000)) {

            NOTE_PIN_HIGH();
            Timer0_us_Wait(note / 2);
            //delay32Us(0, note / 2);

            NOTE_PIN_LOW();
            Timer0_us_Wait(note / 2);
            //delay32Us(0, note / 2);

            t += note;
        }

    }
    else {
    	//NOTE_PIN_LOW();
    	Timer0_Wait(durationMs);
        //delay32Ms(0, durationMs);
    }
}

static void init_ssp(void)
{
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;
	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);

}

static void init_i2c(void)
{
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void init_GPIO(void)
{
	PINSEL_CFG_Type PinCfg;
//	PinCfg.Funcnum = 0;
	PinCfg.Funcnum = 1;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 10;
	PINSEL_ConfigPin(&PinCfg);

	PinCfg.Portnum = 1;
	PinCfg.Pinnum = 31;
	PinCfg.Funcnum = 0;
	PINSEL_ConfigPin(&PinCfg);

	PinCfg.Funcnum = 0;
	PinCfg.Pinnum = 0;
	PinCfg.Portnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	PinCfg.Funcnum = 0;
	PinCfg.Pinnum = 26;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);

	PinCfg.Funcnum = 0;
	PinCfg.Pinnum = 2;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);

	GPIO_SetDir(0, (1<<2), 0);//TEMP sensor
	GPIO_SetDir(2, 1<<10, 0);//SW3
	GPIO_SetDir(1, 1<<31, 0);//SW4
	GPIO_SetDir(2,(1<<0),1);//Red RGB
	GPIO_SetDir(2,(1<<6),1);//Blue RGB

	 GPIO_SetDir(2, 1<<0, 1); //Speaker
	 GPIO_SetDir(2, 1<<1, 1); //Speaker

	 GPIO_SetDir(0, 1<<27, 1); //Speaker
	 GPIO_SetDir(0, 1<<28, 1); //Speaker
	 GPIO_SetDir(2, 1<<13, 1); //Speaker
	 GPIO_SetDir(0, 1<<26, 1); //Speaker

	 GPIO_ClearValue(0, 1<<27); //LM4811-clk
	 GPIO_ClearValue(0, 1<<28); //LM4811-up/dn
	 GPIO_ClearValue(2, 1<<13); //LM4811-shutdn
}

void Temp_int(void)
{
	float temp =0;
	temp=abs(t2-t1);
	TEMP =((temp*100.0)/160.0)-273.15;
}

void UART3_IRQHandler(void){

	INT_COUNT_UART3++;
	UART_Receive(LPC_UART3, &data, 1, NONE_BLOCKING);

	if(INT_COUNT_UART3 == 1){
		if(data == '2'){
			count_log_in++;
		}
	}else if(INT_COUNT_UART3 == 2){
		if(data == '0'){
			count_log_in++;
		}
	}else if(INT_COUNT_UART3 == 3){
		if(data == '2'){
			count_log_in++;
		}
	}else if(INT_COUNT_UART3 == 4){
		if(data == '8'){
			count_log_in++;
		}
	}


	if(INT_COUNT_UART3==4){
		if(count_log_in==4){
		log_in = 1;
		msg = "\r\nWelcome! \r\n";
		UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);

	    msg = "-------------------------Avalon spaceship status--------------------------------\r\n";
	    UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);

	    count_log_in = 0;
	    INT_COUNT_UART3 = 0;
		}else{
		count_log_in = 0;
		INT_COUNT_UART3 = 0;
		msg = "\r\nInvalid password! \r\n";
		UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
	}
	}
	LPC_GPIOINT->IO0IntClr = 1<<2;
}

void EINT0_IRQHandler(void)
{

	// Determine whether GPIO Interrupt P2.10 has occurred
	if ((LPC_GPIOINT->IO2IntStatF>>10)& 0x1)
	{
        MODE_TOGGLE = 1;
        INT_COUNT = INT_COUNT +1;
	}

	LPC_GPIOINT->IO2IntClr = 1<<10;
	LPC_SC -> EXTINT = 0x1;
}

void EINT3_IRQHandler(void)
{

	 if (LPC_GPIOINT->IO0IntStatR>>2)
	{
			j++;
            if(j == 1){
            	t1 = getTicks();
            }
            else if(j==11) {

              t2=getTicks();

              }

            if(j==20)//give the program some computation time
			{
				j=0;
			}
			LPC_GPIOINT->IO0IntClr = 1<<2;

	}
}

void TEMP_WARNING_(void){

	oled_putString(0,45,"Temp. too high\r\n",OLED_COLOR_WHITE,OLED_COLOR_BLACK);

	char my_temp_value[80];
	sprintf(my_temp_value,"%2.2f degrees \n",TEMP);
	oled_putString(0,15,my_temp_value,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
	if(getTicks()-init_t>=333){
		init_t = getTicks();
		RGB = !RGB;
		GPIO_ClearValue(2,(!RGB&1)<<0);
		GPIO_SetValue(2,(RGB&1)<<0);
	}
}

void ACC_WARNING_(void){

	sprintf(display_acc,"X:%2.2f Y:%2.2f\n",x_g,y_g);
	oled_putString(0,30,display_acc,OLED_COLOR_WHITE,OLED_COLOR_BLACK);

	oled_putString(0,55,"Veer off course",OLED_COLOR_WHITE,OLED_COLOR_BLACK);
	if(getTicks()-init_t>=333){
		init_t = getTicks();
		RGB = !RGB;
		GPIO_ClearValue(2,(!RGB&1)<<6);
		GPIO_SetValue(2,(RGB&1)<<6);
	}

}



void STATIONARY(void){

	mode = 1;
	oled_putString(0,0,"STATIONARY",OLED_COLOR_WHITE,OLED_COLOR_BLACK);
	Temp_int();

	if(TEMP>TEMP_HIGH_THRESHOLD){
		TEMP_WARNING = 1;
		countdown = 0;
		i = 15;
		led7seg_setChar(charaters[i],TRUE);

		if(error_message == 0 && log_in == 1){
		msg = "Temp. too high\r\n";
		UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
		error_message++;
		}
	}

	if(CLEAR_WARNING == 0 || mode == 3){
		TEMP_WARNING = 0;
	}

	if(TEMP_WARNING==1){
			TEMP_WARNING_();
	}else if(TEMP!=0){

		GPIO_ClearValue(2,1);
		char my_temp_value[80];
		sprintf(my_temp_value,"%2.2f degrees \n",TEMP);
		oled_putString(0,15,my_temp_value,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
	}
 }

void LAUNCH(void){

	mode = 2;


    char display[80];
    char my_temp_value[80];

    acc_read(&x, &y, &z);
    x_g = (x-xoff)/64.0;
    y_g = (y-yoff)/64.0;

    Temp_int();
    if(log_in==1){
    	if(getTicks()-ini_t_TA>=10000){
    		ini_t_TA = getTicks();
    		if(ACC_WARNING != 1 && TEMP_WARNING != 1){
    		playNote(1912,250);
    		}
    		sprintf(display,"Temp : %2.2f degrees; ACC X : %2.2f, Y : %2.2f \r\n",TEMP,x_g,y_g);
    		UART_Send(LPC_UART3, display , strlen(display), BLOCKING);
    	}
    }

	if(TEMP_WARNING == 1 || ACC_WARNING == 1){
		if(clear_screen == 1){
			oled_clearScreen(OLED_COLOR_BLACK);
			clear_screen++;
		}
	}

	if(TEMP>TEMP_HIGH_THRESHOLD ){
			TEMP_WARNING = 1;
	}

	if(fabs(x_g) > ACC_THRESHOLD || fabs(y_g) > ACC_THRESHOLD ){
		ACC_WARNING = 1;
	}

	if(TEMP_WARNING ==1 && ACC_WARNING==1){

							if(getTicks()-init_t>=333){

								init_t = getTicks();
								RGB = !RGB;

								GPIO_ClearValue(2,(RGB&1)<<6);
		    		    	 	GPIO_SetValue(2,(RGB&1)<<0);

		    		    	 	GPIO_ClearValue(2,(!RGB&1)<<0);
		    		    	 	GPIO_SetValue(2,(!RGB&1)<<6);
							}

							oled_putString(0,0,"LAUNCH",OLED_COLOR_WHITE,OLED_COLOR_BLACK);
							sprintf(my_temp_value,"%2.2f degrees \n",TEMP);
							oled_putString(0,15,my_temp_value,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
							sprintf(display_acc,"X:%2.2f Y:%2.2f\n",x_g,y_g);
							oled_putString(0,30,display_acc,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
							oled_putString(0,45,"Temp. too high",OLED_COLOR_WHITE,OLED_COLOR_BLACK);
							oled_putString(0,55,"Veer off course",OLED_COLOR_WHITE,OLED_COLOR_BLACK);

							if(error_message_TA == 0 && log_in == 1){
			      			msg = "Veer off course.\r\n";
			      			UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);

			      			msg = "Temp. too high\r\n";
			      			UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);

			      			error_message_TA++;
							}
		}else if(TEMP_WARNING==1){

						TEMP_WARNING_();
						oled_putString(0,0,"LAUNCH",OLED_COLOR_WHITE,OLED_COLOR_BLACK);
						sprintf(display_acc,"X:%2.2f Y:%2.2f\n",x_g,y_g);
						oled_putString(0,30,display_acc,OLED_COLOR_WHITE,OLED_COLOR_BLACK);

						if(error_message == 0 && log_in == 1){
						msg = "Temp. too high\r\n";
		      			UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
		      			error_message++;
						}

		}else if(ACC_WARNING==1){

						ACC_WARNING_();
						oled_putString(0,0,"LAUNCH",OLED_COLOR_WHITE,OLED_COLOR_BLACK);
						sprintf(my_temp_value,"%2.2f degrees \n",TEMP);
						oled_putString(0,15,my_temp_value,OLED_COLOR_WHITE,OLED_COLOR_BLACK);

						if(error_message == 0 && log_in == 1){
		      			msg = "Veer off course.\r\n";
		      			UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
		      			error_message++;
						}

		}else{

	oled_putString(0,0,"LAUNCH",OLED_COLOR_WHITE,OLED_COLOR_BLACK);

	sprintf(display_acc,"X:%2.2f Y:%2.2f\n",x_g,y_g);
	oled_putString(0,30,display_acc,OLED_COLOR_WHITE,OLED_COLOR_BLACK);

	sprintf(my_temp_value,"%2.2f degrees \n",TEMP);
	oled_putString(0,15,my_temp_value,OLED_COLOR_WHITE,OLED_COLOR_BLACK);
	}

}

void RETURN(void){

	mode = 3;
	oled_putString(0,0,"RETURN",OLED_COLOR_WHITE,OLED_COLOR_BLACK);
	TEMP_WARNING =0;
	ACC_WARNING=0;

	float NUM_LED;
	uint16_t LED_ARRAY = 0;
	int i;

	if(light_read()<OBST_NEAR_THRESHOLD){
			OBST_WARNING = 1;
			oled_putString(0,15,"Obstacle near",OLED_COLOR_WHITE,OLED_COLOR_BLACK);
			clear_screen=0;
	}else{
		if(clear_screen==0){
			oled_clearScreen(OLED_COLOR_BLACK);
			clear_screen++;
		}
		OBST_WARNING = 0;
	}

			NUM_LED = (light_read()/MAX_LIGHT_INTENSITY)*16.0;
			int TOT_NUM_LED = round(NUM_LED);//Convert float to decimel
			i = 0;
			LED_ARRAY = 0;

			while(i<TOT_NUM_LED){
				LED_ARRAY |= 1<<i;
				i++;
			}
			pca9532_setLeds(LED_ARRAY,0xFFFF);
}


void pinsel_uart3(void){
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 0;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);
}

void init_uart(void){
    UART_CFG_Type uartCfg;
    uartCfg.Baud_rate = 115200;
    uartCfg.Databits = UART_DATABIT_8;
    uartCfg.Parity = UART_PARITY_NONE;
    uartCfg.Stopbits = UART_STOPBIT_1;
    //pin select for uart3;
    pinsel_uart3();
    //supply power & setup working parameters for uart3
    UART_Init(LPC_UART3, &uartCfg);
    //enable transmit for uart3
    UART_TxCmd(LPC_UART3, ENABLE);
}

void func_init(void){

/****************Function initialization******************/
	init_i2c();
    init_ssp();
    led7seg_init();
    rgb_init();

    SysTick_Config(SystemCoreClock/1000);

    init_GPIO();
    acc_init();
    oled_init();
    oled_clearScreen(OLED_COLOR_BLACK);

	light_init();
	light_enable();
	light_setRange(LIGHT_RANGE_4000);
	pca9532_init();

    init_uart();
    pinsel_uart3();

/*********************interrupt**************************/

    LPC_GPIOINT->IO2IntEnF |= 1<<10;
    NVIC_EnableIRQ(EINT0_IRQn);

    LPC_GPIOINT->IO0IntEnR |= 1<<2;
    NVIC_EnableIRQ(EINT3_IRQn);

    NVIC_EnableIRQ(UART3_IRQn);
    UART_IntConfig(LPC_UART3, UART_INTCFG_RBR, ENABLE);

    NVIC_SetPriorityGrouping(5);
    NVIC_SetPriority(SysTick_IRQn,0x00);
    NVIC_SetPriority(UART3_IRQn,0x40);
    NVIC_SetPriority(EINT3_IRQn,0xC0);
    NVIC_SetPriority(EINT0_IRQn,0x80);

    led7seg_setChar(charaters[i],TRUE);
}

int main (void) {

	func_init();
    init_t = getTicks();

    msg = "-----------------------Avalon spaceship login system----------------------------\r\n";
    UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);

    msg = "Please enter the password: \r\n";
    UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);

    while (1)
    {

    	if(MODE_TOGGLE == 1 && mode == 1 && TEMP_WARNING != 1){
      		MODE_TOGGLE_Count++;
      		MODE_TOGGLE = 0;//To reset MODE_TOGGLE
      	}

      	if(getTicks()-PO_init_t >= 1000){//PO:press once
      	      	PO_init_t = getTicks();
      	      	if(MODE_TOGGLE_Count != 1){
      	      		countdown = 0;
      	      		MODE_TOGGLE_Count = 0;
      	      	}else{
      	      		countdown = 1;
      	      		MODE_TOGGLE_Count = 0;
      	      	}
      	}

      	CLEAR_WARNING = GPIO_ReadValue(1)>>31;

       	if(CLEAR_WARNING == 0 ){
       				if(TEMP_WARNING == 1 && ACC_WARNING == 1 && log_in == 1){
            			msg = "Warnings are cleared \r\n";
            			UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
       				}else if((TEMP_WARNING == 1 || ACC_WARNING == 1) && log_in == 1){
            			msg = "Warning is cleared \r\n";
            			UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
       				}

       				TEMP_WARNING = 0;
        			ACC_WARNING = 0;
        			MODE_TOGGLE = 0;
        			countdown = 0;
        			clear_screen = 0;
        			error_message = 0;
        			error_message_TA = 0;
        			GPIO_ClearValue(2,1);
        			GPIO_ClearValue(2,1<<6);
        			oled_clearScreen(OLED_COLOR_BLACK);

        		}

       	if(mode == 1){
      		while(countdown==1 && i!=0){
        	  if(getTicks()-init_t >= 1000){
        		  init_t = getTicks();
        		  if(i != 0){
        			  i--;
        			  led7seg_setChar(charaters[i],TRUE);
        			  if(i<=5){
        			  playNote(1275,250);
        			  }
        		  }
        	  }
        	  STATIONARY();
        	  }

      		if(log_in == 1 && stat_mess_count == 0){
      		msg = "Entering STATIONARY Mode \r\n";
      		UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
      		stat_mess_count++;
      		}

      		STATIONARY();

      		if(buzzer_count==0){
      			if(getTicks()-init_t>=100){
      				init_t = getTicks();
      				 playNote(2272,250);
      				buzzer_count++;
      			}
      		}

      		if(i == 0){
      			oled_clearScreen(OLED_COLOR_BLACK);
      			if(log_in == 1){
      			msg = "Entering LAUNCH Mode \r\n";
      			UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
      			}
      			init_t = getTicks();
      			buzzer_count = 0;
      			countdown = 0;
      			mode = 2;
      			ini_t_TA = getTicks();//

      		}
      		INT_COUNT=0;

       	}else if(mode == 2){
      		LAUNCH();

      		if(buzzer_count<2){

      			 if(getTicks()-init_t>=100){
      				 init_t = getTicks();
      				 playNote(2024,250);
      				 buzzer_count++;
      			    }
      		}

      		if(INT_COUNT == 1){
      			if(getTick_once == 0){
      			ini_t_launch = getTicks();
      			getTick_once++;
      			}
      		}

      		if(INT_COUNT == 2){
      				ini_t_launch_2 = getTicks();
      				if(ini_t_launch_2-ini_t_launch<=1000){

      							INT_COUNT = 0;
      							mode = 3;
      							getTick_once = 0;

      							TEMP_WARNING = 0;
      							ACC_WARNING = 0;
      							clear_screen = 0;
      							oled_clearScreen(OLED_COLOR_BLACK);
      							GPIO_ClearValue(2,1);
      							GPIO_ClearValue(2,1<<6);

      							if(log_in == 1){
      			      			msg = "Entering RETURN Mode \r\n";
      			      			UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
      							}
      			      			error_message = 0;
      			      			error_message_TA = 0;

      			      			buzzer_count = 0;
      			      		    init_t = getTicks();
      			      			PO_init_t = getTicks();


      			}else{
      				INT_COUNT = 0;
      				getTick_once = 0;
      			}
      		}

      		if(INT_COUNT>2){
      			INT_COUNT = 0;
      		}

       	}else if(mode == 3){
      		RETURN();

      		if(buzzer_count<3){
      			if(getTicks()-init_t>=100){
      						init_t = getTicks();
      		      			playNote(1912,250);
      		      			buzzer_count++;
      		    	}
      		 }

         if(getTicks()-PO_init_t >= 1000){
          	PO_init_t = getTicks();

          	if(INT_COUNT!=1){
          		INT_COUNT = 0;
          	}else{
          		mode = 1;//
          		i = 15;
      			INT_COUNT = 0;

      			MODE_TOGGLE = 0;
      			countdown = 0;
      			led7seg_setChar(charaters[i],TRUE);
      			oled_clearScreen(OLED_COLOR_BLACK);

      			if(log_in == 1){
          		msg = "Entering STATIONARY Mode \r\n";
          		UART_Send(LPC_UART3, (uint8_t *)msg , strlen(msg), BLOCKING);
      			}

          		OBST_WARNING = 0;
          		buzzer_count = 0;
          		init_t = getTicks();

          		pca9532_setLeds(0x0000,0xFFFF);
      		}
      	}
      	}
        Timer0_Wait(1);
	}
}


void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}

