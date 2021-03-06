/*****************************************************************************
 *   A demo example using several of the peripherals on the base board
 *
 *   Copyright(C) 2011, EE2024
 *   All rights reserved.
 *
 ******************************************************************************/
#include "stdio.h"

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h" // for delay

#include "joystick.h"
#include "pca9532.h"
#include "acc.h"
#include "oled.h"
#include "rgb.h"
#include "led7seg.h"

typedef enum {
	Initialization, Climb, Emergency
}MachineState; //MachineState is a type

MachineState state = Initialization;

//Setting Specifications
static const int LIGHT_THRESHOLD = 300; 	//in lux
static const int TEMP_THRESHOLD = 28;	//in degree C
static const double ACC_THRESHOLD = 0.1;	//in g

static uint8_t barPos = 2;

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
     *
     * "E2,F4,"
     */
    while(*song != '\0') {
        note = getNote(*song++);
        if (*song == '\0')
            break;
        dur  = getDuration(*song++);
        if (*song == '\0')
            break;
        pause = getPause(*song++);

        playNote(note, dur);
        //delay32Ms(0, pause);
        Timer0_Wait(pause);
    }
}

static uint8_t * song = (uint8_t*)"D4";//"C2.C2,D4,C4,F4,E8,";
        //(uint8_t*)"C2.C2,D4,C4,F4,E8,C2.C2,D4,C4,G4,F8,C2.C2,c4,A4,F4,E4,D4,A2.A2,H4,F4,G4,F8,";
        //"D4,B4,B4,A4,A4,G4,E4,D4.D2,E4,E4,A4,F4,D8.D4,d4,d4,c4,c4,B4,G4,E4.E2,F4,F4,A4,A4,G8,";

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

	//SW3
	PinCfg.Portnum = 1; //port 1
	PinCfg.Pinnum = 31; // pin 31
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1<<31), 0); //portNum 1, pin 31 (sw4 PiO2_9 mapped to P1.31), 0 for input

	//SW4
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
}

static void init_everything(){
	init_i2c();
	init_ssp();
	init_GPIO();

    pca9532_init();
    joystick_init();
    acc_init();
    oled_init();
    led7seg_init();
    speaker_init();
}

//Interrupt Handler
void EINT3_IRQHandler(void){ //for interrupts
	// SW3
	printf("SW3 is pressed\n");
	LPC_GPIOINT ->IO0IntClr = 1<<4; //clear the interrupt
	//MODE_TOGGLE is pressed in Initialization mode > 7seg countdown from 9 to 0 inclusive (decrease every 500ms), blue LED to behave as in BLINK_BLUE
	//>OLED display “INITIALIZATION COMPLETE. ENTERING CLIMB MODE”
	//>RGB LED should blink as described in BLINK_BLUE
	//After, enter CLIMB mode
	state = Climb;
}

//RGB LEDs
void ALTERNATE_LED(){
	//The blue and red LEDs alternate every 500 milliseconds. The Green LED should be off throughout.
}
void BLINK_BLUE(){
	//Blue Light for RGB LED, alternating between ON and OFF every 1 second
}

//Modes
void do_Initialization(){
	printf("Entered Initialization Mode");
	//display "Initialization mode. Press TOGGLE to climb"
	oled_putString(0, 0, (uint8_t *) "Initialization mode. Press TOGGLE to climb", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	//initialise the interrupt for MODE_TOGGLE with SW3
	LPC_GPIOINT ->IO0IntEnR |= 1<<4;
	NVIC_EnableIRQ(EINT3_IRQn);
}
void do_Climb(){
	printf("Entered Climb Mode");
	//OLED display "CLIMB"
	oled_putString(0, 0, (uint8_t *) "CLIMB", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	//output the net acceleration on the OLED screen. The accelerometer readings should be initialized to be close to zero when the device enters CLIMB Mode.
	//EMERGENCY Mode may be triggered through Fall Detection (shaking the board gently, net acceleration> ACC_THRESHOLD) in CLIMB Mode. No other mode should be able to trigger EMERGENCY Mode.
	//if net acceleration> ACC_THRESHOLD{ state = Emergency;}
	//temperature sensor should output the temperature reading (to 1 decimal place) on the OLED screen in the following format: "Temp: xx.x deg" where xx.x is the temperature reading in oC accurate to 1 decimal place.
	//If the temperature crosses TEMP_THRESHOLD, the OLED screen should show ‘REST NOW’ for 3 seconds before returning to CLIMB Mode. This should only be triggered once every time the temperature crosses TEMP_THRESHOLD. If the temperature still remains above TEMP_THRESHOLD after 3 seconds, the alert should NOT be triggered again, unless the temperature goes below TEMP_THRESHOLD and crosses it again.
	//light sensor should be continuously read and the reading printed on the OLED display in the following format: "Light: xx lux" where xx is the reading.
	//If the light sensor reading falls below LIGHT_THRESHOLD, the lights on LED_ARRAY should light up proportionately to how low the ambient light is (i.e., the dimmer the ambient light, the more the number of LEDs that should be lit). A message should also be displayed on the OLED screen saying "DIM"
	//If the light sensor reading is above LIGHT_THRESHOLD, LED_ARRAY should not be lit.
	//The accelerometer, temperature and light sensor readings should be sent to FiTrackX once every 5 seconds.
}
void do_Emergency(){
	printf("Entered Emergency Mode");
	//OLED screen should display "EMERGENCY Mode!" as well as the net acceleration, the temperature as well as the duration for which FitNUS has been in EMERGENCY Mode.
	oled_putString(0, 0, (uint8_t *) "CLIMB", OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	//RGB LED should behave as described in ALTERNATE_LED
	ALTERNATE_LED();
	//send a message to FiTrackX that reads “EMERGENCY!"
	//Every 5 seconds, FitNUS should send the accelerometer and temperature sensor readings as well as the time elapsed since entering EMERGENCY Mode to FiTrackX.
	//MODE_TOGGLE and EMERGENCY_OVER are simultaneously pressed >  send the message: "Emergency is cleared! Time consumed for recovery: xx sec", where xx is the time elapsed since entering EMERGENCY Mode
	//5-second duration should elapse before the device enters CLIMB Mode automatically. During these 5 seconds, the 7-segment display should show the letters S-A-U-E-D, with each letter changing every 1 second
	//and RGB_LED should behave as described in BLINK_BLUE. (V cannot be displayed on the 7-segment, so it is replaced with U').
	BLINK_BLUE();
}

//Functions for testing devices
/* ####### Joystick and 7seg  ###### */
void Joystick_7seg(uint8_t joyState){
	if (joyState >> 0 & 0x01) //JOYSTICK_CENTER
		led7seg_setChar('C', 0);
	if (joyState >> 1 & 0x01) //JOYSTICK_UP
		led7seg_setChar('U', 0);
	if (joyState >> 2 & 0x01) //JOYSTICK_DOWN
		led7seg_setChar('D', 0);
	if (joyState >> 3 & 0x01) //JOYSTICK_LEFT
		led7seg_setChar('L', 0);
	if (joyState >> 4 & 0x01) //JOYSTICK_RIGHT
		led7seg_setChar('R', 0);
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

int main (void) {
	init_everything();

    int32_t xoff = 0;
    int32_t yoff = 0;
    int32_t zoff = 0;
    int8_t x = 0;
    int8_t y = 0;
    int8_t z = 0;

    uint8_t dir = 1;
    uint8_t wait = 0;

    uint8_t joyState = 0;

    uint8_t btn1 = 1;

    /*
     * Assume base board in zero-g position when reading first value.
     */
    acc_read(&x, &y, &z);
    xoff = 0-x;
    yoff = 0-y;
    zoff = 64-z;

    /* ---- Speaker ------> */
    GPIO_SetDir(2, 1<<0, 1);
    GPIO_SetDir(2, 1<<1, 1);

    /* <---- OLED ------ */
    moveBar(1, dir);
    oled_clearScreen(OLED_COLOR_BLACK);

    while (1){
    	if (state == Initialization){
    		do_Initialization();
    	}
    	if (state == Climb){
    		do_Climb();
		}
    	if (state == Emergency){
			do_Emergency();
		}

        /* #Testing functions */
        /* ############################################# */
        Joystick_7seg(joyState);
        Joystick_OLED(joyState);
        Accelerometer_LED(x, y, z, xoff, yoff, zoff, dir, wait);
        SW_Speaker(btn1);
        Trimpot_RGB();
        /* ############################################# */

        /* # */
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
