/* $Id: phonecall.c,v 1.1 2001/10/25 06:49:17 micahjd Exp $
 *
 * phonecall.c - GUI and data structures to represent information about one
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

#include <picogui.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include "phonecall.h"

/****************************************** Globals */

pghandle wInfoBar;   /* Call info toolbar, 0 if hidden */
pghandle wCallStatus, wPhoneNumber, wName, wConnectTime;

/* Strings representing call status */
const char *status_messages[] = {
  "Call in progress",
  "Incoming call",
  "Call on hold",
  "Call muted",
  "Call completed"
};

struct phonecall *current_call;

/* Previous value for connection time */
time_t old_connect_time;

/****************************************** Public functions */

/* Show and hide the call info bar */
void show_call_info(struct phonecall *call) {
  
  if (!wInfoBar) {

    /* Keep everything in a context so we can easily show and hide the info */
    pgEnterContext();

    /* If we don't have an info bar yet, create it */
    wInfoBar = pgRegisterApp(PG_APP_TOOLBAR,"Phone Call Info",
			     PG_APPSPEC_SIDE, PG_S_BOTTOM,
			     0);

    /* Create widgets within the toolbar */

    wPhoneNumber = pgNewWidget(PG_WIDGET_LABEL,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE,PG_S_LEFT,
		PG_WP_FONT,pgNewFont(NULL,14,PG_FSTYLE_BOLD),
		PG_WP_TEXT,pgNewString(" "),
		0);

    /* Make the connect time opaque and fixed-width to minimize the
     * amount of redrawing necessary to update it */
    wConnectTime = pgNewWidget(PG_WIDGET_LABEL,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_TRANSPARENT,0,
		PG_WP_SIDE,PG_S_RIGHT,
		PG_WP_FONT,pgNewFont(NULL,0,PG_FSTYLE_FIXED),
		0);
    wCallStatus = pgNewWidget(PG_WIDGET_LABEL,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE,PG_S_RIGHT,
		0);

    wName = pgNewWidget(PG_WIDGET_LABEL,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE,PG_S_ALL,  /* Center in remaining space */
		PG_WP_FONT,pgNewFont(NULL,12,0),
		0);
  }

  current_call = call;
  old_connect_time = -1;

  /* Display info on this call 
   *
   * FIXME: pretty-print the phone number, so the user sees "(303) 555-1234"
   *        instead of "3035551234". Make sure to look up the appropriate specs
   *        so it does local calls, long distance, 10-digit dialing,
   *        and international correctly.
   */
  set_call_status(call,call->status);
  update_call_timer();
  pgSetIdle(500,update_call_timer);
  pgUpdate();
}

void hide_call_info(void) {
  if (!wInfoBar)
    return;

  /* Leave our context to destroy the info bar and all associated handles */
  pgSetIdle(0,NULL);
  pgLeaveContext();
  wInfoBar = 0;
  pgUpdate();
}

/* Create a new phone call */
struct phonecall *new_call(int status) {
  struct phonecall *c;

  /* Allocate new call */
  c = malloc(sizeof(struct phonecall));
  if (!c)
    return NULL;
  memset(c,0,sizeof(struct phonecall));

  /* Fill in values */
  set_call_status(c,status);

  return c;
}

/* Update the name and number of the current call. If either is NULL, the
 * existing value is retained.
 */
void set_call_id(struct phonecall *call, const char *name, 
		 const char *number) {
  if (name) {
    strncpy(call->name,name,MAX_NAME_LEN);
    pgReplaceText(wName,call->name);
  }
  if (number) {
    strncpy(call->number,number,MAX_PHONENUM_LEN);
    pgReplaceText(wPhoneNumber,call->number);
  }
}

/* Update the status of the current call,
 * setting time stamps when applicable */
void set_call_status(struct phonecall *call,int status) {
  if (!call)
    return;
  call->original_status = call->status = status;

  if (!call->begin_time && status==CALL_INPROGRESS)
    call->begin_time = time(NULL);
  if (!call->end_time && status==CALL_COMPLETED)
    call->end_time = time(NULL);

  if (wInfoBar && call==current_call) {
    pgReplaceText(wCallStatus,status_messages[call->status]);
    pgUpdate();
  }
}

/* Adds to the end of the call's phone number */
void call_dial(struct phonecall *call, char digit) {
  char str[2] = " ";
  *str = digit;

  /* Already full? */
  if (strlen(call->number)>=MAX_PHONENUM_LEN-1)
    return;
  
  strcat(call->number,str);
  if (wInfoBar && call==current_call) {
    pgReplaceText(wPhoneNumber,call->number);
    pgUpdate();
  }
}

/* Update the 'connect time' display, if applicable.
 * This should be called at least every second when a call is active.
 * Calling it more often or when a call isn't active won't hurt.
 */
void update_call_timer(void) {
  time_t now, starttime, duration;

  phone_check_ring_timeout();
  
  if (!current_call)
    return;
  if (!wInfoBar)
    return;

  /* Measure the time from connect to now or 
   * (if the call has ended) it's end */
  starttime = current_call->begin_time;
  now = time(NULL);
  if (!starttime)
    starttime = now;
  if (current_call->end_time)
    now = current_call->end_time;
  duration = now - starttime;

  /* If there's a call in progress, don't let the power management kick in */
  if (current_call->begin_time && !current_call->end_time)
    pgSetInactivity(0);

  if (old_connect_time != duration) {
    pgReplaceTextFmt(wConnectTime,"%2d:%02d", duration/60,duration%60);
    pgSubUpdate(wConnectTime);
  }

  old_connect_time = duration;
}

/****************************************** Utilities */


/* The End */
