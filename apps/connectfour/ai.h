/*
 * Filename:  ai.h
 * Author:    Brandon Smith
 * Date:      April 1, 2002
 * Purpose:   A header file for the main AI file
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

#ifndef __AI_H__
#define __AI_H__

#include "connectfour.h"
#include "rules.h"

void aicall(struct board *it);
void randommove(struct board *it);
int move(struct board *it, int location);

#endif  /* __AI_H__ */

