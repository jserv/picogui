/* $Id$
 *
 * phonecall.h - GUI and data structures to represent information about one
 *               phone call
 *
 * Copyright (C) 2001 Micah Dowty <micahjd@users.sourceforge.net>
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
 */

#ifndef _H_PG_PHONECALL
#define _H_PG_PHONECALL

#include <time.h>   /* For time_t */

/* Call status */
#define CALL_INPROGRESS   0   /* Call in progress */
#define CALL_INCOMING     1   /* Incoming call, phone ringing */
#define CALL_HOLD         2   /* Call has been accepted, but on hold */
#define CALL_MUTE         3   /* Call is muted */
#define CALL_COMPLETED    4   /* Call is completed, completion time is valid */

/* These limits are sufficent, per the callerid spec */
#define MAX_PHONENUM_LEN  32
#define MAX_NAME_LEN      32

/* Info about one phone call */
struct phonecall {
  int original_status;               /* For distinguishing incoming calls */
  int status;                        /* A CALL_* constant */
  char number[MAX_PHONENUM_LEN];     /* phone number */
  char name[MAX_NAME_LEN];           /* From caller-id (or address book?) */
  time_t begin_time;                 /* Time when the call was initiated */
  time_t end_time;                   /* Time when the call was completed */
};

extern struct phonecall *current_call;

/* Show and hide the call info bar */
void init_call_info(void);
void show_call_info(struct phonecall *call);
void hide_call_info(void);

/* Create a new phone call */
struct phonecall *new_call(int status);

/* Number to redial */
extern char redial_number[MAX_PHONENUM_LEN];

/* Save a completed call in the history database  */
void archive_call(struct phonecall *call);

/* Update the name and number of the current call. If either is NULL, the
 * existing value is retained.
 */
void set_call_id(struct phonecall *call, const char *name, const char *number);

/* Update the status of the current call,
 * setting time stamps when applicable */
void set_call_status(struct phonecall *call, int status);

/* Adds to the end of the call's phone number */
void call_dial(struct phonecall *call, char digit);

/* Update the 'connect time' display, if applicable.
 * This should be called at least every second when a call is active.
 * Calling it more often or when a call isn't active won't hurt.
 */
void update_call_timer(void);

#endif /* _H_PG_PHONECALL */

/* The End */
