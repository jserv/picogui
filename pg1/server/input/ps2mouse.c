/* $Id$
 *
 * ps2mouse.c - Driver for PS/2 compatible mouse devices, including input core
 *              drivers that emulate PS/2 mice.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 *  This contains a lot of bits from GPM.
 *  GPM is licenced under the GPL, and it is copyrighted by:
 *    Copyright (C) 1993        Andrew Haylett <ajh@gec-mrc.co.uk>
 *    Copyright (C) 1994-2000   Alessandro Rubini <rubini@linux.it>
 *    Copyright (C) 1998,1999   Ian Zimmerman <itz@rahul.net>
 *    Copyright (C) 2001        Nico Schottelius <nicos@pcsystems.de>
 *
 *
 */
#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/configfile.h>

#ifdef DEBUG_EVENT
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#include <unistd.h>
#include <termios.h>
#include <stdio.h>

int ps2_fd;
int ps2_packetlen;
struct cursor *ps2_cursor;

/* From GPM's headers/defines.h */

#define GPM_AUX_SEND_ID    0xF2
#define GPM_AUX_ID_ERROR   -1
#define GPM_AUX_ID_PS2     0
#define GPM_AUX_ID_IMPS2   3

#define GPM_AUX_SET_RES        0xE8  /* Set resolution */
#define GPM_AUX_SET_SCALE11    0xE6  /* Set 1:1 scaling */ 
#define GPM_AUX_SET_SCALE21    0xE7  /* Set 2:1 scaling */
#define GPM_AUX_GET_SCALE      0xE9  /* Get scaling factor */
#define GPM_AUX_SET_STREAM     0xEA  /* Set stream mode */
#define GPM_AUX_SET_SAMPLE     0xF3  /* Set sample rate */ 
#define GPM_AUX_ENABLE_DEV     0xF4  /* Enable aux device */
#define GPM_AUX_DISABLE_DEV    0xF5  /* Disable aux device */
#define GPM_AUX_RESET          0xFF  /* Reset aux device */
#define GPM_AUX_ACK            0xFA  /* Command byte ACK. */ 

/*
 * Sends the SEND_ID command to the ps2-type mouse.
 * Return one of GPM_AUX_ID_...
 */
static int read_mouse_id(int fd)
 {
  unsigned char c = GPM_AUX_SEND_ID;
  unsigned char id;

  write(fd, &c, 1);
  read(fd, &c, 1);
  if (c != GPM_AUX_ACK) {
    return(GPM_AUX_ID_ERROR);
  }
  read(fd, &id, 1);

  return(id);
}

/*
 * Writes the given data to the ps2-type mouse.
 * Checks for an ACK from each byte.
 * 
 * Returns 0 if OK, or >0 if 1 or more errors occurred.
 */
static int write_to_mouse(int fd, unsigned char *data, size_t len)
{
  int i;
  int error = 0;
  for (i = 0; i < len; i++) {
    unsigned char c;
    write(fd, &data[i], 1);
    read(fd, &c, 1);
    if (c != GPM_AUX_ACK) {
      error++;
    }
  }

  /* flush any left-over input */
  usleep (30000);
  tcflush (fd, TCIFLUSH);
  return(error);
}

/* Most of this is borrowed from the serialmouse driver
 */
int ps2mouse_fd_activate(int fd) {
  u8 buttons;
  s8 dx,dy,dz;
  static u8 packet[3];
  static int pos;

  if (fd != ps2_fd)
    return 0;

  /* Read a correctly-aligned mouse packet. If the first byte isn't 0x40,
   * it isn't correctly aligned. The mouse packet is 3 or 4 bytes long.
   * On fast machines, we can't read a whole packet at once, so we have
   * to maintain the state in a static variable.
   */
  
  if (read(ps2_fd,packet+pos,1) != 1)
    return 1;
  if (!(packet[0] & 0x08))
    return 1;
  if (++pos < ps2_packetlen)
    return 1;
  pos = 0;
   
  /* Decode the packet (dx/dy ripped from GPM) */
  
  buttons = packet[0] & 7;

  /* Some PS/2 mice send reports with negative bit set in data[0]
   * and zero for movement.  I think this is a bug in the mouse, but
   * working around it only causes artifacts when the actual report is -256;
   * they'll be treated as zero. This should be rare if the mouse sampling
   * rate is set to a reasonable value; the default of 100 Hz is plenty.
   * (Stephen Tell)
   */
  if(packet[1] != 0)
    dx = (packet[0] & 0x10) ? packet[1]-256 : packet[1];
  else
    dx = 0;
  if(packet[2] != 0)
    dy = -((packet[0] & 0x20) ? packet[2]-256 : packet[2]);
  else
    dy = 0;

  /* Optional Z axis */
  if (ps2_packetlen >= 4)
    dz = (s8) packet[3];
  else
    dz = 0;

  infilter_send_pointing(PG_TRIGGER_PNTR_RELATIVE, dx, dy, buttons, ps2_cursor);
  infilter_send_pointing(PG_TRIGGER_SCROLLWHEEL, 0, dz, 0, ps2_cursor);
  
  return 1;
}

g_error ps2mouse_init(void) {
  const char *mousedev;
  static unsigned char basic_init[] = { GPM_AUX_ENABLE_DEV, GPM_AUX_SET_SAMPLE, 100 };
  static unsigned char imps2_init[] = { GPM_AUX_SET_SAMPLE, 200, GPM_AUX_SET_SAMPLE, 100, GPM_AUX_SET_SAMPLE, 80, };
  static unsigned char ps2_init[] = { GPM_AUX_SET_SCALE11, GPM_AUX_ENABLE_DEV, GPM_AUX_SET_SAMPLE, 100, GPM_AUX_SET_RES, 3, };
  int id;
  int noinit = get_param_int("input-ps2mouse","noinit",0);
  int flags;
  g_error e;

  if (noinit)
    flags = O_RDONLY | O_NDELAY;
  else
    flags = O_RDWR | O_NDELAY;

  mousedev = get_param_str("input-ps2mouse","device",NULL);
  if (mousedev) {
    ps2_fd = open(mousedev,flags);
  }
  else {
    ps2_fd = open("/dev/input/mice",flags);
    if (ps2_fd < 0)
      ps2_fd = open("/dev/psaux",flags);
  }

  if(ps2_fd < 0)
    return mkerror(PG_ERRT_IO,43);   /* Error opening mouse device */

  if (!noinit) {

    /* Do a basic init in case the mouse is confused */
    write_to_mouse(ps2_fd, basic_init, sizeof (basic_init));
    
    /* Now try again and make sure we have a PS/2 mouse */
    if (write_to_mouse(ps2_fd, basic_init, sizeof (basic_init)) != 0) {
      DBG("imps2: PS/2 mouse failed init\n");
      return mkerror(PG_ERRT_IO,74);   /* Error initializing mouse */
    }
    
    /* Try to switch to 3 button mode */
    if (write_to_mouse(ps2_fd, imps2_init, sizeof (imps2_init)) != 0) {
      DBG("imps2: PS/2 mouse failed (3 button) init\n");
      return mkerror(PG_ERRT_IO,74);   /* Error initializing mouse */
    }
    
    /* Read the mouse id */
    id = read_mouse_id(ps2_fd);
    if (id == GPM_AUX_ID_ERROR) {
      DBG("imps2: PS/2 mouse failed to read id, assuming standard PS/2\n");
      id = GPM_AUX_ID_PS2;
    }
    
    /* And do the real initialisation */
    if (write_to_mouse(ps2_fd, ps2_init, sizeof (ps2_init)) != 0) {
      DBG("imps2: PS/2 mouse failed setup, continuing...\n");
    }
    
    if (id == GPM_AUX_ID_IMPS2) {
      /* Really an intellipoint, so initialise 3 button mode (4 byte packets) */
      DBG("imps2: Auto-detected intellimouse PS/2\n");
      ps2_packetlen = 4;
    }
    else {
      /* Some other kind of mouse, assume 3 byte packets */
      ps2_packetlen = 3;
      
#ifdef DEBUG_FILE
      if (id != GPM_AUX_ID_PS2) {
	DBG("imps2: Auto-detected unknown mouse type %d, assuming standard PS/2\n", id);
      }
      else {
	DBG("imps2: Auto-detected standard PS/2\n");
      }
#endif
    }    
  }
  else {
    DBG("skipping PS/2 mouse initialization, assuming standard ps2 mouse\n");
    ps2_packetlen = 3;
  }    

  e = cursor_new(&ps2_cursor,NULL,-1);
  errorcheck;

  return success;
}

void ps2mouse_fd_init(int *n,fd_set *readfds,struct
		     timeval *timeout) {
   if ((*n)<(ps2_fd+1))
     *n = ps2_fd+1;
   if (ps2_fd>0)
     FD_SET(ps2_fd,readfds);
}

void ps2mouse_close(void){
  pointer_free(-1,ps2_cursor);
  close(ps2_fd);
}

g_error ps2mouse_regfunc(struct inlib *i) {
  i->init = &ps2mouse_init;
  i->fd_activate = &ps2mouse_fd_activate;
  i->fd_init = &ps2mouse_fd_init;
  i->close = &ps2mouse_close;
  return success;
}

/* The End */
