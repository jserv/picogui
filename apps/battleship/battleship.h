/*
 * Filename: battleship.h
 *
 * Copyright (C) Brandon Smith 2000 (lottabs2@yahoo.com)
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
 * Contributers
 *
 *
 *
 *
 */


#ifndef __BATTLESHIP_H__
#define __BATTLESHIP_H__

/****************** The board defines ****************/

/***************
 *  * These values are arbitrary, and can be changed without
 *   * any problems.  They just define the first paramater in
 *    * the board array, so the values must be 0 - 3.
 *     *
 *      * Pretty simple huh?
 *       * ***************/

#define COMP_GUESS 0
#define COMP_PIECE 1
#define PLAY_GUESS 2
#define PLAY_PIECE 3

/*************** The piece defines ***************** */

/**************
 *  * These values are the length of each of the ships.
 *   * The program can't tell the difference between a
 *    * Cruiser and a Submarine, but it is all preprosser
 *     * stuff anyway, so who cares?
 *      * I fixed it so it can tell the difference, and it isn't hard.
 *       * The ship name is the length in all cases except the cruiser.
 *        * Therefore, the number is the ID number.
 *         * the HIT_$(SHIPNAME) is the value for printing a hit piece.
 *          *************/

#define SHIP_HIT 0xFF0000
#define SHIP_SUNK 0xFF1F11
#define CARRIER 5
#define CARRIER_COLOR 0x777777
#define HIT_CARRIER 10
#define HIT_CARRIER_COLOR SHIP_HIT
#define SUNK_CARRIER -10
#define SUNK_CARRIER_COLOR SHIP_SUNK
#define BATTLESHIP 4
#define BATTLESHIP_COLOR 0x777777
#define HIT_BATTLESHIP 9
#define HIT_BATTLESHIP_COLOR HIT_CARRIER_COLOR
#define SUNK_BATTLESHIP -9
#define SUNK_BATTLESHIP_COLOR SUNK_CARRIER_COLOR
#define CRUISER 1
#define CRUISER_COLOR 0x777777
#define HIT_CRUISER 6
#define HIT_CRUISER_COLOR SHIP_HIT
#define SUNK_CRUISER -6
#define SUNK_CRUISER_COLOR SHIP_SUNK
#define SUBMARINE 3
#define SUBMARINE_COLOR 0x777777
#define HIT_SUBMARINE 8
#define HIT_SUBMARINE_COLOR SHIP_HIT
#define SUNK_SUBMARINE -8
#define SUNK_SUBMARINE_COLOR SHIP_SUNK
#define DESTROYER 2
#define DESTROYER_COLOR 0x777777
#define HIT_DESTROYER 7
#define HIT_DESTROYER_COLOR SHIP_HIT
#define SUNK_DESTROYER -7
#define SUNK_DESTROYER_COLOR SHIP_SUNK
#define SUNK -100
/* defines for the shooting parts*/

/****************
 *  * These values are used for the shooting part of the game
 *   * HIT and MISS are obvious, and the remaining defines are for
 *    * the simple AI player
 *     ***************/

#define MISS -1
#define HIT -2

/* This is a bit of a joke, don't pay attention */
#define NOLL 0


/* Color defines for the canvas widget stuff */

#define OCEAN_COLOR 0x070717
#define HIT_COLOR 0xFF1F11
#define MISS_COLOR 0xFFFFFF
#define SHIP_COLOR 0xFFFF1F
#define BACKGROUND_COLOR 0xFFFFFF

typedef struct status
{
        int status;
        int letter;
        int number;
        int play_points;
        int comp_points;

        /* Stores the state of the player's ships for the player's knowledge
	 * and the number of hits each ship has taken for the output.
	 * The AI will only look to see if the number indicates the ship is sunk
	 */
        int play_ships[6];

        /* Stores the state of the comp's ships for the player.
	 *          * It only stores the ships as they are sunk,
         */
        int comp_ships[6];

}status;

int evt_new_game(struct pgEvent *evt);
int settings(struct pgEvent *evt);
int clickski(struct pgEvent *evt);
int clickski2(struct pgEvent *evt);
int redraw(struct pgEvent *evt);
void draw_spot(int x, int y, int type);
void comp_draw_spot(int x, int y, int type);
void aicall(void);
void quoe(int x);
void comp_ship_place(int size);

void draw_miss(int x, int y);
void draw_hit(int x, int y);
void draw_ship(int x, int y, int side);
void draw_hit_ship(int x, int y);


#endif /* __BATTLESHIP_H__ */
