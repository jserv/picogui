/*********************************************************
 * wizard.h
 *
 * Copyright (c) Stefan Hellkvist
 *
 * The functions every wizard should implement
 * See elasticWizard.c for an example of an implementation
 *********************************************************/

#ifndef WIZARD_H
#define WIZARD_H

#include <pattern.h>

typedef enum { NATURAL, NUMERAL, EXTENDED } BANK;

BANK getBank();
void setBank( BANK b );
void initWizard();
unsigned char recognize( Pattern *p );


#endif
