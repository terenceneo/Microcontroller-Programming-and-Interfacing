/*****************************************************************************
 *   Assignment 2
 *
 *	 Neo Yu Yao Terence, Neo Jia Wen Rachel
 *   Copyright(C) 2018, EE2028
 *   All rights reserved.
 *
 ******************************************************************************/
#include "stdio.h"
#include "math.h"
#include "string.h"

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h" // for delay
#include "lpc17xx_uart.h"

#include "joystick.h"
#include "pca9532.h"
#include "acc.h"
#include "oled.h"
#include "rgb.h"
#include "led7seg.h"
#include "temp.h"
#include "light.h"

typedef enum {
	Initialization, Climb, Emergency, ItoC, Emergency_over
}MachineState; //MachineState is a type

typedef enum {
	None, Dim, Rest, Music
}ClimbState; //ClimbState is a type

MachineState 		state = Initialization;
ClimbState			Climb_State = None;

//Setting Specifications
static const int 		LIGHT_THRESHOLD = 300; 	//in lux
static const uint32_t 	TEMP_THRESHOLD = 260;	//28 degree C
static const double 	ACC_THRESHOLD = 0.5;	//in g ///original threshold at 0.1

static uint8_t numbers_inverted[] = {0x24, 0x7D, 0xE0, 0x70, 0x39, 0x32, 0x23, 0x7C, 0x20, 0x38, 0xFF};
char saued[] = "SAUED"; // change to array of inverted letters

// each tone in a song is a note, duration and pause eg. C2. > note=C, duration=2, pause=.
static uint8_t * song = (uint8_t*)"D4,";//"C2.C2,D4,C4,F4,E8,";
        //(uint8_t*)"C2.C2,D4,C4,F4,E8,C2.C2,D4,C4,G4,F8,C2.C2,c4,A4,F4,E4,D4,A2.A2,H4,F4,G4,F8,";
        //"D4,B4,B4,A4,A4,G4,E4,D4.D2,E4,E4,A4,F4,D8.D4,d4,d4,c4,c4,B4,G4,E4.E2,F4,F4,A4,A4,G8,";

static uint32_t notes[] = {
        2272, // A - 440 Hz
        2024, // B - 494 Hz
        3816, // C - 262 Hz
        3401, // D - 294 Hz
        3030, // E - 330 Hz
        2865, // F - 349 Hz
        2551, // G - 392 Hz
        1136, // a - 880 Hz
        1012, // b - 988 Hz
        1912, // c - 523 Hz
        1703, // d - 587 Hz
        1517, // e - 659 Hz
        1432, // f - 698 Hz
        1275, // g - 784 Hz
};

uint8_t 	restnow_OLED_line = 32;
uint8_t 	dim_OLED_line = 40; //OLED line to print DIM in climb mode

static uint8_t barPos = 2;

//Variables
//Timing variables
volatile uint32_t 	msTicks = 0; 	// counter for 1ms SysTicks
volatile uint32_t 	getTicks = 0; 	//what is this used for? //////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t 			sensor_refresh_ticks = 200;
uint32_t 			prev_sensor_ticks;
uint32_t 			prev_alternateled_ticks;
uint32_t 			prev_blink_blue_ticks;
uint32_t 			prev_countdown_ticks;
uint32_t 			prev_saved_ticks;
uint32_t 			prev_temp_ticks;
uint32_t 			prev_uart_ticks;

uint32_t 			emer_start_ticks;
uint32_t 			emer_dur = 0;

//Flags
uint8_t 			msFlag = 0; 	//used in countdown, can be removed
uint8_t 			SevenSegFlag = 9;
uint8_t 			RGB_FLAG = 0;
uint8_t 			countdown_flag = 0;

uint8_t 			temp_flag = 0;
uint8_t 			restnow_printed = 0;

//Counters
int 				saved_count = 0;

//Sensor input storage variables
volatile uint32_t 	tempvalue = 0;
volatile uint32_t 	luminI;

int32_t 			xoff = 0;
int32_t 			yoff = 0;
int32_t 			zoff = 0;
int8_t 				x = 0;
int8_t 				y = 0;
int8_t 				z = 0;
float 				net_acc = 0;

uint8_t 			dir = 1;
uint8_t 			wait = 0;

uint8_t 			joyState = 0;

uint8_t 			btn1 = 1;

//Peripheral output storage variables
uint32_t ledOn = 0x0;
int 	shift = 0;
char 	uart_msg[50];
char 	temp_string[32];

static void moveBar(uint8_t steps, uint8_t dir){
    uint16_t ledOn = 0;

    if (barPos == 0)
        ledOn = (1 << 0) | (3 << 14);
    else if (barPos == 1)
        ledOn = (3 << 0) | (1 << 15);
    else
        ledOn = 0x07 << (barPos-2);

    barPos += (dir*steps);
    barPos = (barPos % 16);

    pca9532_setLeds(ledOn, 0xffff);
}


static void drawOled(uint8_t joyState)
{
    static int wait = 0;
    static uint8_t currX = 48;
    static uint8_t currY = 32;
    static uint8_t lastX = 0;
    static uint8_t lastY = 0;

    if ((joyState & JOYSTICK_CENTER) != 0) {
        oled_clearScreen(OLED_COLOR_BLACK);
        return;
    }
    if (wait++ < 3)
        return;
    wait = 0;
    if ((joyState & JOYSTICK_UP) != 0 && currY > 0) {
        currY--;
    }
    if ((joyState & JOYSTICK_DOWN) != 0 && currY < OLED_DISPLAY_HEIGHT-1) {
        currY++;
    }
    if ((joyState & JOYSTICK_RIGHT) != 0 && currX < OLED_DISPLAY_WIDTH-1) {
        currX++;
    }
    if ((joyState & JOYSTICK_LEFT) != 0 && currX > 0) {
        currX--;
    }
    if (lastX != currX || lastY != currY) {
        oled_putPixel(currX, currY, OLED_COLOR_WHITE);
        lastX = currX;
        lastY = currY;
    }
}


#define NOTE_PIN_HIGH() GPIO_SetValue(0, 1<<26);
#define NOTE_PIN_LOW()  GPIO_ClearValue(0, 1<<26);

uint32_t Get_Time(void){
	return msTicks;
}

static void playNote(uint32_t note, uint32_t durationMs) {
    uint32_t t = 0;
    if (note > 0) {
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
    	Timer0_Wait(durationMs);
        //delay32Ms(0, durationMs);
    }
}

static uint32_t getNote(uint8_t ch){
    if (ch >= 'A' && ch <= 'G')
        return notes[ch - 'A'];

    if (ch >= 'a' && ch <= 'g')
        return notes[ch - 'a' + 7];

    return 0;
}

static uint32_t getDuration(uint8_t ch){
    if (ch < '0' || ch > '9')
        return 400;
    /* number of ms */
    return (ch - '0') * 200;
}

static uint32_t getPause(uint8_t ch)
{
    switch (ch) {
    case '+': 	return 0;
    case ',': 	return 5;
    case '.': 	return 20;
    case '_': 	return 30;
    default: 	return 5;
    }
}

static void playSong(uint8_t *song) {
    uint32_t note = 0;
    uint32_t dur  = 0;
    uint32_t pause = 0;

    /*
     * A song is a collection of tones where each tone is
     * a note, duration and pause, e.g.
     * "E2,F4,"
     */
    while(*song != '\0') {
        note = getNote(*song++); //eg. E
        if (*song == '\0')
            break;
        dur  = getDuration(*song++); //eg. 2
        if (*song == '\0')
            break;
        pause = getPause(*song++); //eg. ,

        playNote(note, dur);
        //delay32Ms(0, pause);
        Timer0_Wait(pause);
    }
}

static void init_ssp(void){
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

static void init_i2c(void){
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	//similar to GPIO set direction
	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void init_GPIO(void){
	// Initialize button ////////////////////////////// was blank
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 0; // For GPIO

	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;

	//SW4
	PinCfg.Portnum = 1; //port 1
	PinCfg.Pinnum = 31; // pin 31
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1<<31), 0); //portNum 1, pin 31 (sw4 PiO2_9 mapped to P1.31), 0 for input

	//SW3
	PinCfg.Portnum = 0; //port 0
	PinCfg.Pinnum = 4; // pin 4
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<4), 0); //portNum 0, pin 4 (sw3 BL_EN mapped to P0.4), 0 for input
}

static void speaker_init(){
	PINSEL_CFG_Type PinCfg;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;

	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 26;
	PINSEL_ConfigPin(&PinCfg);

	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 27;
	PINSEL_ConfigPin(&PinCfg);

	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 28;
	PINSEL_ConfigPin(&PinCfg);

	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 13;
	PINSEL_ConfigPin(&PinCfg);

    GPIO_SetDir(0, 1<<27, 1);
    GPIO_SetDir(0, 1<<28, 1);
    GPIO_SetDir(2, 1<<13, 1);
    GPIO_SetDir(0, 1<<26, 1);

    //Clear P0.27, P0.28, P2.13, as we are not doing Volume Control or shutdown
    GPIO_ClearValue(0, 1<<27); //LM4811-clk
    GPIO_ClearValue(0, 1<<28); //LM4811-up/dn
    GPIO_ClearValue(2, 1<<13); //LM4811-shutdn

        /* ---- Speaker from demo code------> */
	GPIO_SetDir(2, 1<<0, 1);
	GPIO_SetDir(2, 1<<1, 1);
}

void lightSenIntInit(){
	PINSEL_CFG_Type PinCfg;

	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 5;

	PINSEL_ConfigPin(&PinCfg);

	GPIO_SetDir(2, (1<<5), 0);

	light_clearIrqStatus(); //Good to clear when initializing
	light_setLoThreshold(LIGHT_THRESHOLD); //lower threshold for
}

void init_uart(void){
	UART_CFG_Type uartCfg;
	uartCfg.Baud_rate = 115200; //baud rate to match terminal programme
	uartCfg.Databits = UART_DATABIT_8; //number of bits in each data packet
	uartCfg.Parity = UART_PARITY_NONE; //for error checking
	uartCfg.Stopbits = UART_STOPBIT_1; //number of bits to use for stop

	//pin select for uart3
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0; //need these?
	PinCfg.Pinmode = 0; //need these?
	PinCfg.Pinnum = 0;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);

	//supply power and setup working parts for uart3
	UART_Init(LPC_UART3, &uartCfg);
	//enable transmit for uart3
	UART_TxCmd(LPC_UART3, ENABLE);
	//enable UART Rx interrupt
	UART_IntConfig(LPC_UART3, UART_INTCFG_RBR, ENABLE);
	//enable Interrupt for UART3
	NVIC_EnableIRQ(UART3_IRQn);
}

static void init_everything(){
	init_i2c();
	init_ssp();
	init_GPIO();
	init_uart();

	SysTick_Config(SystemCoreClock/1000);
	temp_init(&Get_Time);

    pca9532_init();
    joystick_init();
    acc_init();
    oled_init();
    led7seg_init();
    speaker_init();
    rgb_init();
    lightSenIntInit();

    LPC_GPIOINT ->IO0IntEnF |= 1<<4; //sw3
//    LPC_GPIOINT ->IO1IntEnR |= 1<<31; //sw4
    LPC_GPIOINT ->IO2IntEnF |= 1<<5; //light sensor, activates on falling edge as light sensor is active low
    NVIC_EnableIRQ(EINT3_IRQn);
}

//RGB LEDs
//Function to deconflict PIO1_10, Port 2 Pin one used by RGB_GREEN and OLED
void setRGBLeds (uint8_t ledMask){
    if ((ledMask & RGB_RED) != 0) {
        GPIO_SetValue( 2, (1<<0) );
    } else {
        GPIO_ClearValue( 2, (1<<0) );
    }
    if ((ledMask & RGB_BLUE) != 0) {
        GPIO_SetValue( 0, (1<<26) );
    } else {
        GPIO_ClearValue( 0, (1<<26) );
    }
}

void ALTERNATE_LED(){
	//The blue and red LEDs alternate every 500 milliseconds. The Green LED should be off throughout.
	if(Get_Time() - prev_alternateled_ticks >= 500){
		if(RGB_FLAG == 0){
			setRGBLeds (RGB_BLUE);
			RGB_FLAG = 1;
			prev_alternateled_ticks = Get_Time();
//			printf("Blue: %d\n", prev_alternateled_ticks);
		}else{
			setRGBLeds (RGB_RED); // Clear value for RGB_BLUE
			RGB_FLAG = 0;
			prev_alternateled_ticks = Get_Time();
//			printf("Red: %d\n", prev_alternateled_ticks);
		}
	}
}

void BLINK_BLUE (void){
	//Blue Light for RGB LED, alternating between ON and OFF every 1 second
	if(Get_Time() - prev_blink_blue_ticks >= 1000){
//		printf("blinked blue\n");
		if(RGB_FLAG == 0){
			setRGBLeds (RGB_BLUE);
			RGB_FLAG = 1;
			prev_blink_blue_ticks = Get_Time();
		}else{
			setRGBLeds (0); // Clear value for RGB_BLUE
			RGB_FLAG = 0;
			prev_blink_blue_ticks = Get_Time();
		}
	}
}

//void countdown(void){
//	while(1){
//		if (msFlag == 0){
//			if (msTicks%500 == 0){
//				BLINK_BLUE();
//				switch (SevenSegFlag){
//					case 9:
//						led7seg_setChar(0x38, TRUE);
//						SevenSegFlag = 8;
//						break;
//					case 8:
//						led7seg_setChar(0x20, TRUE);
//						SevenSegFlag = 7;
//						break;
//					case 7:
//						led7seg_setChar(0x7C, TRUE);
//						SevenSegFlag = 6;
//						break;
//					case 6:
//						led7seg_setChar(0x23, TRUE);
//						SevenSegFlag = 5;
//						break;
//					case 5:
//						led7seg_setChar(0x32, TRUE);
//						SevenSegFlag = 4;
//						break;
//					case 4:
//						led7seg_setChar(0x39, TRUE);
//						SevenSegFlag = 3;
//						break;
//					case 3:
//						led7seg_setChar(0x70, TRUE);
//						SevenSegFlag = 2;
//						break;
//					case 2:
//						led7seg_setChar(0xE0, TRUE);
//						SevenSegFlag = 1;
//						break;
//					case 1:
//						led7seg_setChar(0x7D, TRUE);
//						SevenSegFlag = 0;
//						break;
//					case 0:
//						led7seg_setChar(0x24, TRUE);
//						SevenSegFlag = 10;
//						break;
//					default:
//						led7seg_setChar(0xFF, TRUE);
//						SevenSegFlag = 9;
//						return;
//				}
//				msFlag = 1;
//			}
//		}
//		else{
//			if(msTicks%500 != 0){
//				msFlag= 0;
//			}
//		}
//	}
//}

void countdown_new(void){
	if(Get_Time() - prev_countdown_ticks >= 500){
//		printf("counted down\n");
		led7seg_setChar(numbers_inverted[SevenSegFlag], TRUE);
		prev_countdown_ticks = Get_Time();
		if (SevenSegFlag == 0){
			SevenSegFlag = 9;
//				led7seg_setChar(numbers_inverted[SevenSegFlag], TRUE);
			state = Climb;
			printf("State changed from ItoC to Climb\n");
			return;
		}
		SevenSegFlag -- ;
	}
}

void saved(void){
	//5-second duration should elapse before the device enters CLIMB Mode automatically.
	//During these 5 seconds, the 7-segment display should show the letters S-A-U-E-D, with each letter changing every 1 second (V cannot be displayed on the 7-segment, so it is replaced with U').
	if(Get_Time() - prev_saved_ticks >= 1000){
		led7seg_setChar(saued[saved_count], FALSE); //change to true after changing saued to array
		prev_saved_ticks = Get_Time();
		if (saved_count == 4){
			saved_count = 0;
			state = Climb;
			printf("State changed from Emergency_over to Climb\n");
			return;
		}
		saved_count ++ ;
	}
}

void uart_Send(char* msg){
	int len = strlen(msg);
	UART_Send(LPC_UART3, (uint8_t*)msg, (uint32_t)len, BLOCKING);
}

//void UARTMessages(void){
//
//}

//Modes
void do_Initialization(){
	printf("Entered Initialization Mode\n");
	//display "Initialization mode. Press TOGGLE to climb"
	oled_clearScreen(OLED_COLOR_BLACK);
	oled_putString(0, 0, (uint8_t *) "Initialization", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 8, (uint8_t *) "mode. Press", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 16, (uint8_t *) "TOGGLE to climb", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	while(state == Initialization){}
	//MODE_TOGGLE with SW3 to ItoC via interrupt
}

void do_toclimb(){
	printf("Entered ItoC Mode\n");
	//>OLED display “INITIALIZATION COMPLETE. ENTERING CLIMB MODE”
	oled_clearScreen(OLED_COLOR_BLACK);
	oled_putString(0, 0, (uint8_t *) "INITIALIZATION", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 8, (uint8_t *) "COMPLETE.", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	oled_putString(0, 16, (uint8_t *) "ENTERING CLIMB MODE", OLED_COLOR_WHITE, OLED_COLOR_BLACK);

	prev_countdown_ticks
	= prev_blink_blue_ticks = Get_Time();

	while(state == ItoC){
		//MODE_TOGGLE is pressed in Initialization mode > 7seg countdown from 9 to 0 inclusive (decrease every 500ms)
		countdown_new();
		//>RGB LED should blink as described in BLINK_BLUE
		BLINK_BLUE();
		//After, enter CLIMB mode //Called in countdown_new()
	}
}

void do_Climb(){
	//rgb_setLeds(RGB_GREEN); /////////////////////////////////////////////////////////////////////////////////////////what is this line?
	printf("Entered Climb Mode\n");
	led7seg_setChar(0xFF, TRUE);
	GPIO_ClearValue( 0, (1<<26) ); // Clear value for RGB_BLUE
	//OLED display "CLIMB"
	oled_clearScreen(OLED_COLOR_BLACK);
	oled_putString(0, 0, (uint8_t *) "CLIMB", OLED_COLOR_WHITE, OLED_COLOR_BLACK);

	//The accelerometer readings should be initialized to be close to zero when the device enters CLIMB Mode.
	acc_read(&x, &y, &z);
	xoff = -x;
	yoff = -y;
	zoff = -z;
	
	prev_sensor_ticks
	= prev_uart_ticks = Get_Time();

	while(state == Climb){
		//The net acceleration (to 1 decimal place) should be displayed on the OLED screen in the following format: "Acc: x.xx" where x.xx is the net acceleration accurate to 2 decimal places, in 'g's (1g = 9.8m/s2).
		acc_read(&x, &y, &z);
		x = x+xoff;
		y = y+yoff;
		z = z+zoff;
		net_acc = (sqrt(x*x + y*y + z*z))/ 64;
		//EMERGENCY Mode may be triggered through Fall Detection (shaking the board gently, net acceleration> ACC_THRESHOLD) in CLIMB Mode. No other mode should be able to trigger EMERGENCY Mode.
		if(net_acc > ACC_THRESHOLD){
			printf("State changed from Climb to Emergency\n");
			state = Emergency;
		}

		//Refresh
		if ((Get_Time() - prev_sensor_ticks) >= sensor_refresh_ticks){
			//temperature sensor should output the temperature reading (to 1 decimal place) on the OLED screen in the following format: "Temp: xx.x deg" where xx.x is the temperature reading in oC accurate to 1 decimal place.
			tempvalue = temp_read(); //gives 10* temperature in degree C
			sprintf(temp_string,"Temp: %lu.%lu deg", tempvalue/10, tempvalue%10);
			oled_putString(0, 16, (uint8_t *) temp_string, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

			sprintf(temp_string,"Acc: %5.2f g", net_acc);
			oled_putString(0, 8, (uint8_t *) temp_string, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

			prev_sensor_ticks = Get_Time();
		}

		//If the temperature crosses TEMP_THRESHOLD, the OLED screen should show ‘REST NOW’ for 3 seconds before returning to CLIMB Mode. This should only be triggered once every time the temperature crosses TEMP_THRESHOLD.
		if (tempvalue > TEMP_THRESHOLD && temp_flag == 0){
			prev_temp_ticks = Get_Time();
			oled_clearScreen(OLED_COLOR_BLACK);
			oled_putString(0, restnow_OLED_line, (uint8_t *) "REST NOW", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
			//If the temperature still remains above TEMP_THRESHOLD after 3 seconds, the alert should NOT be triggered again
			temp_flag = 1;
			restnow_printed = 1;
		}
		//unless the temperature goes below TEMP_THRESHOLD and crosses it again.
		if (tempvalue <= TEMP_THRESHOLD && temp_flag == 1){
			temp_flag = 0;
		}
		
		//light sensor should be continuously read and the reading printed on the OLED display in the following format: "Light: xx lux" where xx is the reading.
		light_enable();
		luminI = light_read();
		sprintf(temp_string,"Light: %4lu lux\n",luminI);
		oled_putString(0, 24, (uint8_t *) temp_string, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
		//If the light sensor reading falls below LIGHT_THRESHOLD, the lights on LED_ARRAY should light up proportionately to how low the ambient light is (i.e., the dimmer the ambient light, the more the number of LEDs that should be lit).
		//If the light sensor reading is above LIGHT_THRESHOLD, LED_ARRAY should not be lit.
		shift = luminI / 18.75;
		ledOn = (shift <= 16)?(1 << (16-shift)) - 1: 0x0; //max 1<<16 - 1 = 0xffff (16 1's), min 1<<0 - 1 = 0
//		if (shift <= 16){ ledOn = (1 << (16-shift)) - 1;}
//		else ledOn = 0x0;
		pca9532_setLeds(ledOn, 0xffff); // turns on ledOn and off everything else, ledOn takes priority
		
		//A message should also be displayed on the OLED screen saying "DIM"
		if(luminI < LIGHT_THRESHOLD) oled_putString(0, dim_OLED_line, (uint8_t *) "DIM", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
		else oled_fillRect(0, dim_OLED_line, OLED_DISPLAY_WIDTH, dim_OLED_line+8, OLED_COLOR_BLACK); //clear dim_OLED_line
		
		// the OLED screen should show ‘REST NOW’ for 3 seconds before returning to CLIMB Mode.
		if (restnow_printed && Get_Time() - prev_temp_ticks >= 3000){
			oled_fillRect(0, restnow_OLED_line, OLED_DISPLAY_WIDTH, restnow_OLED_line+8, OLED_COLOR_BLACK); //clear restnow_OLED_line
			restnow_printed = 0;
		}	

		//The accelerometer, temperature and light sensor readings should be sent to FiTrackX once every 5 seconds.
//		if ((Get_Time() - prev_uart_ticks) >= 5000){
//			sprintf(uart_msg,"Temp: %lu.%lu deg", tempvalue/10, tempvalue%10); //to be edited
//			uart_Send(uart_msg);
//			prev_uart_ticks = Get_Time();
//		}
	}
}

void do_Emergency(){
	printf("Entered Emergency Mode\n");
	//OLED screen should display "EMERGENCY Mode!"
	oled_clearScreen(OLED_COLOR_BLACK);
	oled_putString(0, 0, (uint8_t *) "EMERGENCY Mode!", OLED_COLOR_WHITE, OLED_COLOR_BLACK);

	prev_blink_blue_ticks
	= prev_alternateled_ticks
	= emer_start_ticks
	= prev_sensor_ticks
	= prev_uart_ticks = Get_Time();

	//send a message to FiTrackX that reads “EMERGENCY!"
	uart_Send("EMERGENCY!");

	while(state == Emergency){
		//Refresh
		if ((Get_Time() - prev_sensor_ticks) >= sensor_refresh_ticks){
			//as well as the net acceleration, the temperature as well as the duration for which FitNUS has been in EMERGENCY Mode.
			acc_read(&x, &y, &z);
			x = x+xoff;
			y = y+yoff;
			z = z+zoff;
			net_acc = (sqrt(x*x + y*y + z*z)) / 64;
			sprintf(temp_string,"Acc: %5.2f g", net_acc);
			oled_putString(0, 8, (uint8_t *) temp_string, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

			tempvalue = temp_read(); //gives 10* temperature in degree C
			sprintf(temp_string,"Temp: %lu.%lu deg", tempvalue/10, tempvalue%10);
			oled_putString(0, 16, (uint8_t *) temp_string, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

			prev_sensor_ticks = Get_Time();
		}

		if (Get_Time() - emer_start_ticks >= 1000){
			emer_dur ++;
			emer_start_ticks = Get_Time();
			sprintf(temp_string,"Dur: %lu s", emer_dur);
			oled_putString(0, 24, (uint8_t *) temp_string, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
		}

		//RGB LED should behave as described in ALTERNATE_LED
		ALTERNATE_LED();
		//Every 5 seconds, FitNUS should send the accelerometer and temperature sensor readings as well as the time elapsed since entering EMERGENCY Mode to FiTrackX.
//		if ((Get_Time() - prev_uart_ticks) >= 5000){
//			sprintf(uart_msg,"Temp: %lu.%lu deg", tempvalue/10, tempvalue%10); //to be edited
//			uart_Send(uart_msg);
//			prev_uart_ticks = Get_Time();
//		}
	}
}

void do_Emergency_over(){
	printf("Entered Emergency_over Mode\n");
	oled_clearScreen(OLED_COLOR_BLACK);
	//MODE_TOGGLE and EMERGENCY_OVER are simultaneously pressed
	//>  send the message: "Emergency is cleared! Time consumed for recovery: xx sec", where xx is the time elapsed since entering EMERGENCY Mode
	//duration has been saved in emer_dur
	sprintf(uart_msg,"Emergency is cleared! Time consumed for recovery: %lu sec", emer_dur); //to be edited
	uart_Send(uart_msg);

	prev_saved_ticks
	= prev_blink_blue_ticks = Get_Time();

	while (state == Emergency_over){
		//5-second duration should elapse before the device enters CLIMB Mode automatically.
		//During these 5 seconds, the 7-segment display should show the letters S-A-U-E-D, with each letter changing every 1 second (V cannot be displayed on the 7-segment, so it is replaced with U').
		saved();
		//and RGB_LED should behave as described in BLINK_BLUE.
		BLINK_BLUE();
	}
}

//Functions for testing devices
/* ####### Joystick and 7seg  ###### */
void Joystick_7seg(uint8_t joyState){
	if (joyState >> 0 & 0x01) //JOYSTICK_CENTER
		led7seg_setChar('C', FALSE);
	if (joyState >> 1 & 0x01) //JOYSTICK_UP
		led7seg_setChar('U', FALSE);
	if (joyState >> 2 & 0x01) //JOYSTICK_DOWN
		led7seg_setChar('D', FALSE);
	if (joyState >> 3 & 0x01) //JOYSTICK_LEFT
		led7seg_setChar('L', FALSE);
	if (joyState >> 4 & 0x01) //JOYSTICK_RIGHT
		led7seg_setChar('R', FALSE);
}

/* ####### Joystick and OLED  ###### */
void Joystick_OLED(uint8_t joyState){
	joyState = joystick_read();
	if (joyState != 0)
		drawOled(joyState);
}

/* ####### Accelerometer and LEDs  ###### */
void Accelerometer_LED(int8_t x, int8_t y, int8_t z, int8_t xoff, int8_t yoff, int8_t zoff, uint8_t dir, uint8_t wait){
	acc_read(&x, &y, &z);
	x = x+xoff;
	y = y+yoff;
	z = z+zoff;

	if (y < 0) {
		dir = 1;
		y = -y;
	}
	else {
		dir = -1;
	}
	if (y > 1 && wait++ > (40 / (1 + (y/10)))) {
		moveBar(1, dir);
		wait = 0;
	}
}
/* ####### SW3 and Speaker  ###### */
void SW_Speaker(uint8_t btn1){
	btn1 = (GPIO_ReadValue(1) >> 31) & 0x01; // reading from SW3
	if (btn1 == 0){
		playSong(song);
	}
}

/* ############ Trimpot and RGB LED  ########### */
void Trimpot_RGB(){}

//Interrupt Handler
void EINT3_IRQHandler(void){ //for interrupts
	// SW3
	if ((LPC_GPIOINT ->IO0IntStatF>>4) & 0x1){ //sw3
		LPC_GPIOINT ->IO0IntClr = 1<<4; //clear the interrupt
		printf("SW3 is pressed\n");
		if (state == Initialization){
			state = ItoC;
			printf("State changed from Initialization to ItoC\n");
		}else if (state == Climb){
			state = Initialization;
			printf("State changed to Initialization\n");
		}else if (state == Emergency && (((GPIO_ReadValue(1) >> 31) & 0x1) == 0)){ //sw4 ==0 when pressed
//			state = Initialization;
//			printf("State changed to Initialization\n");
			printf("%d\n", ((GPIO_ReadValue(1) >> 31) & 0x1) == 0);
			state = Emergency_over;
			printf("State changed to Emergency_over\n");
		}
	//	do_toclimb();
	}
	if ((LPC_GPIOINT ->IO2IntStatF>>5) & 0x1){ //light sensor
		LPC_GPIOINT ->IO2IntClr = 1<<5; //clear the interrupt
		printf("Light interrupt triggered\n");
		light_clearIrqStatus();
	}
}

//UART3 interrupt handler
void UART3_IRQHandler(void){
    UART3_StdIntHandler();
}

//Handler occurs every 1ms
void SysTick_Handler (void){
	msTicks++;
}

int main (void) {
//	if (SysTick_Config(SystemCoreClock/1000)){
//		while (1);
//	}
	init_everything();

    /*
     * Assume base board in zero-g position when reading first value.
     */
    acc_read(&x, &y, &z);
    xoff = 0-x;
    yoff = 0-y;
    zoff = 64-z;
    moveBar(1, dir);

    /* <---- OLED ------ */
    oled_clearScreen(OLED_COLOR_BLACK);

    led7seg_setChar(0xFF, TRUE);
    while (1){
    	if (state == Initialization){
    		do_Initialization();
//    		printf("Main i\n");
    	}
    	if (state == ItoC){
    		do_toclimb();
//    		printf("Main itoc\n");
    	}
    	if (state == Climb){
    		do_Climb();
//    		printf("Main Climb\n");
		}
    	if (state == Emergency){
			do_Emergency();
//			printf("Main E\n");
		}
    	if (state == Emergency_over){
    		do_Emergency_over();
    	}

        /* #Testing functions */
        /* ############################################# */
//        Joystick_7seg(joyState);
//        Joystick_OLED(joyState);
//        Accelerometer_LED(x, y, z, xoff, yoff, zoff, dir, wait);
//        SW_Speaker(btn1);
//        Trimpot_RGB();
        /* ############################################# */

        /* # */
//        Timer0_Wait(1);
    }
}

void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}
