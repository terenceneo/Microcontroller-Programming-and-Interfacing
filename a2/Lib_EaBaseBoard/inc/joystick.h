/*****************************************************************************
 *   light.h:  Header file for the joystick switch
 *
 *   Copyright(C) 2009, Embedded Artists AB
 *   All rights reserved.
 *
******************************************************************************/
#ifndef __JOYSTICK_H
#define __JOYSTICK_H


#define JOYSTICK_CENTER 0x01 //bit0
#define JOYSTICK_UP     0x02 //bit1
#define JOYSTICK_DOWN   0x04 //bit2
#define JOYSTICK_LEFT   0x08 //bit3
#define JOYSTICK_RIGHT  0x10 //bit4


void joystick_init (void);
uint8_t joystick_read(void);



#endif /* end __JOYSTICK_H */
/****************************************************************************
**                            End Of File
*****************************************************************************/
