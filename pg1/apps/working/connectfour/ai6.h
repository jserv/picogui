/*
 * Filename:  ai6.h
 * Author:    Brandon Smith
 * Date:      April 6, 2002
 * Purpose:   Header FILE (go josh) go file go...
 *
 * Copyright (C) 2002 Brandon Smith <lottabs2@yahoo.com> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 *
 */




#ifndef __AI6_H__
#define __AI6_H__

#include "connectfour.h"

void ai6(struct board *it);

int gentrap(struct board *it);
int gentrapwin(struct board *it);
int gentraplose(struct board *it);

int spotwin(struct board *it, int x, int y, int player);
int gmask(struct board *it, int maskx, int masky, int x, int y);
int maskout(int mask, int val);

#endif /* __AI6_H__ */
