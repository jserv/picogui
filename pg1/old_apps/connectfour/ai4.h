/*
 * Filename:  ai4.h
 * Author:    Brandon Smith
 * Date:      April 2, 2002
 * Purpose:   the fourth graduation of the AI
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




#ifndef __AI4_H__
#define __AI4_H__

#include "connectfour.h"

void ai4(struct board *it);

int linetrap(struct board *it);
int linetrapwin(struct board *it);
int linetraplose(struct board *it);
int linetrapgetleft(struct board *it);

#endif /* __AI4_H__ */
