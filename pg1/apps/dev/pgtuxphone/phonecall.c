/* $Id$
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
#include "phoneui.h"

/****************************************** Globals */

pghandle wInfoBar;   /* Call info toolbar, 0 if hidden */
pghandle wCallStatus, wPhoneNumber, wName, wConnectTime;
pghandle wKeypadBtn;

/* Strings representing call status */
const char *status_messages[] = {
  "Call in progress",
  "Incoming call",
  "Call on hold",
  "Call muted",
  "Call completed"
};

struct phonecall *current_call;
char redial_number[MAX_PHONENUM_LEN] = "";

/* Previous value for connection time */
time_t old_connect_time;

/* Arrow bitmap: 11x6 1bpp PNM file */
unsigned char arrow_bits[] = {
0x50, 0x34, 0x0A, 0x0A, 0x31, 0x31, 0x20, 0x36, 0x0A, 0x04, 
0x00, 0x0E, 0x00, 0x1F, 0x00, 0x3F, 0x80, 0x7F, 0xC0, 0xFF, 
0xE0, 
};
#define arrow_len 21

/* Functions to create a VFD-style display and set it's text */
void new_vfd_label(pghandle canvas);
void set_vfd_text(pghandle vfd,const char *text); 

/****************************************** Public functions */

/* Create our toolbar */
void init_call_info(void) {
  pghandle bArrowMask,bArrow;
    
  /* Set up a bitmap/bitmask for the keypad button up arrow.
   * We have the mask stored in PNM format, and we paint
   * the arrow bitmap itself solid black. If we wanted another
   * color for the arrow, we could paint it that other color,
   * then apply the bitmask to it with the PG_LGOP_INVERT_AND
   * logical operation to mask out only the arrow part.
   */
  bArrowMask = pgNewBitmap(pgFromMemory(arrow_bits,arrow_len));
  bArrow = pgCreateBitmap(11,6);
  pgRender(bArrow,PG_GROP_SETCOLOR,0x000000);
  pgRender(bArrow,PG_GROP_RECT,0,0,15,8);

  /* Create a toolbar that starts out hidden */
  wInfoBar = pgRegisterApp(PG_APP_TOOLBAR,"pgtuxphone/call_info",
			   PG_APPSPEC_SIDE, PG_S_BOTTOM,
			   0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,0,
	      0);
  
  /* Create widgets within the toolbar */
  
  wKeypadBtn = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_BITMAP, bArrow,
	      PG_WP_BITMASK, bArrowMask,
	      PG_WP_EXTDEVENTS, PG_EXEV_TOGGLE,
	      PG_WP_TEXT,pgNewString("Keypad"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnKeypad,NULL);
  
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_TEXT,pgNewString("Redial"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnRedial,NULL);

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
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_FONT,pgNewFont(NULL,12,0),
	      0);

  wPhoneNumber = pgNewWidget(PG_WIDGET_CANVAS,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);
  new_vfd_label(wPhoneNumber);

}

/* Show and hide the call info bar */
void show_call_info(struct phonecall *call) {
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
  pgSetWidget(wKeypadBtn,
	      PG_WP_ON,0,
	      0);
  set_vfd_text(wPhoneNumber,"");
  pgSetWidget(wInfoBar,
	      PG_WP_SIZE,-1,    /* Automatic sizing */
	      0);
  pgUpdate();
}

void hide_call_info(void) {
  /* Turn off the idle handler, hide everything */
  pgSetIdle(0,NULL);
  pgSetWidget(wInfoBar,PG_WP_SIZE,0,0);
  pgSetWidget(wKeypad,PG_WP_SIZE,0,0);
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

/* FIXME: We're not doing any database things yet, this just stores
 * the number so we can do a redial
 */
void archive_call(struct phonecall *call) {
  strcpy(redial_number,call->number);
  free(call);
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
    set_vfd_text(wPhoneNumber,call->number);
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

  if (call==current_call) {
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
  if (call==current_call) {
    set_vfd_text(wPhoneNumber,call->number);
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

/* Functions to create a VFD-style display and set it's text */
void new_vfd_label(pghandle canvas) {
  pgcontext gc;
  pghandle h = pgNewString(" ");

  /* Use a canvas widget to do a 'inverse VFD style' phone number display 
   *
   * Code like this makes me really glad that the canvas widget is
   * as smart as it is about scaling and calculating preferred sizes
   */
  gc = pgNewCanvasContext(canvas,PGFX_PERSISTENT);
  pgSetMapping(gc,0,0,1,1,PG_MAP_SCALE);
  pgSetColor(gc,0xBFEDBD);   /* Pastel green */
  pgRect(gc,0,0,1,1);
  pgSetColor(gc,0x000000);
  pgFrame(gc,0,0,1,1);
  pgSetFont(gc,pgNewFont(NULL,20,0));
  pgText(gc,0,0,h);
  pgSetPayload(canvas,h);
  pgDeleteContext(gc);  
}

void set_vfd_text(pghandle vfd,const char *text) {
  /* Use the lower level canvas commands to change the VFD's text.
   * The display's previous text is stored as its payload so we can
   * delete it, and we set the new text by changing the text gropnode's
   * parameter
   */

  pghandle h;
  pgDelete(pgGetPayload(vfd));
  h = pgNewString(text);
  pgWriteCmd(vfd,PGCANVAS_SETGROP,1,h);
  pgWriteCmd(vfd,PGCANVAS_REDRAW,0);
  pgSetPayload(vfd,h);
}

/* The End */
