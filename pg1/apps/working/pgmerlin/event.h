/*********************************************************
 * event.h
 *
 * Copyright (c) Stefan Hellkvist
 *********************************************************/

#ifndef EVENT_H
#define EVENT_H

#include <wizard.h>

#define PEN_DRAG  0
#define PEN_UP    1
#define PEN_DOWN  2

typedef struct PenEvent
{
    int x, y;
} PenEvent;


typedef enum { SINGLE_STROKE, MULTIPLE_STROKE } MODE;

extern BANK bank;
extern MODE mode;
extern int lastWasShift;
extern int timer;

void setMode( MODE m );
void eventLoop( void );


#endif



