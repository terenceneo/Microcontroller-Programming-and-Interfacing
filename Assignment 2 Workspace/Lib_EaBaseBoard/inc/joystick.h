/*****************************************************************************
 *   light.h:  Header file for the joystick switch
 *
 *   Copyright(C) 2009, Embedded Artists AB
 *   All rights reserved.
 *
******************************************************************************/
#ifndef __JOYSTICK_H
#define __JOYSTICK_H


#define JOYSTICK_CENTER 0x01 //1<<0
#define JOYSTICK_UP     0x02 //1<<1
#define JOYSTICK_DOWN   0x04 //1<<2
#define JOYSTICK_LEFT   0x08 //1<<3
#define JOYSTICK_RIGHT  0x10 //1<<4


void joystick_init (void);
uint8_t joystick_read(void);



#endif /* end __JOYSTICK_H */
/****************************************************************************
**                            End Of File
*****************************************************************************/
