/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <stdio.h>
#include <errno.h>

#include <picogui.h>
#include <picogui/pgkeys.h>
#include "../pgboard/pgboard.h"
#include "pics.h"

typedef enum
{
  SHIFT_CHAR = 1,
  NUMBER_CHAR,
  SYMBOL_CHAR,
  UNSHIFT_CHAR,
  BACKSPACE_CHAR = PGKEY_BACKSPACE,
  SPACE_CHAR = ' '
}
special_chars;

typedef enum
{
  LOWERCASE_MODE = 0,
  UPPERCASE_MODE,
  NUMBER_MODE,
  SYMBOL_MODE
}
entry_modes;

typedef enum
{
  MODE_NORMAL = 0,
  MODE_ONCE,
  MODE_LOCK
}
entry_mode_state;

static int char_sets[4][32] = {
  {'a', 's', 'k', NUMBER_CHAR, BACKSPACE_CHAR, SHIFT_CHAR, 'p', 'f', 'n', 'l',
   'x', 'u', 't', 'y', 'j', 'r', 'i', 'd', 'b', SYMBOL_CHAR, SPACE_CHAR,
   UNSHIFT_CHAR, 'z', 'g', 'o', 'w', 'v', 'c', 'e', 'h', 'q', 'm'},
  {'A', 'S', 'K', NUMBER_CHAR, BACKSPACE_CHAR, SHIFT_CHAR, 'P', 'F', 'N', 'L',
   'X', 'U', 'T', 'Y', 'J', 'R', 'I', 'D', 'B', SYMBOL_CHAR, SPACE_CHAR,
   UNSHIFT_CHAR, 'Z', 'G', 'O', 'W', 'V', 'C', 'E', 'H', 'Q', 'M'},
  {'0', '<', '>', NUMBER_CHAR, BACKSPACE_CHAR, SHIFT_CHAR, '(', ')', ',', '[',
   ']', '+', '=', '-', '9', '8', '.', '%', '$', SYMBOL_CHAR, SPACE_CHAR,
   UNSHIFT_CHAR, '7', '6', '5', '*', '/', '4', '3', '#', '2', '1'},
  {'/', '\\', '|', NUMBER_CHAR, BACKSPACE_CHAR, SHIFT_CHAR, '(', ')', ',',
   '[', ']', '"', '\'', '&', '!', '?', '.', '-', '_', SYMBOL_CHAR, SPACE_CHAR,
   UNSHIFT_CHAR, '}', '{', '*', '*', '^', ':', ';', '#', ' ', '@'}
};

static int tap_keys[9] = {
  0,
  0,
  PGKEY_LALT,			/* Alt */
  PGKEY_RETURN,			/* Ret */
  PGKEY_BACKSPACE,		/* Bks */
  0,
  PGKEY_ESCAPE,			/* Esc */
  PGKEY_TAB,			/* Tab */
  PGKEY_LCTRL			/* Ctl */
};

int zone_data[32][3] = {
  /* 0  */ {6, 0, 0},
  /* 1  */ {6, 5, 0},
  /* 2  */ {6, 5, 4},
  /* 3  */ {5, 6, 0},
  /* 4  */ {5, 0, 0},
  /* 5  */ {5, 4, 0},
  /* 6  */ {4, 5, 6},
  /* 7  */ {4, 5, 0},
  /* 8  */ {4, 0, 0},
  /* 9  */ {4, 3, 0},
  /* 10 */ {4, 3, 2},
  /* 11 */ {3, 4, 0},
  /* 12 */ {3, 0, 0},
  /* 13 */ {3, 2, 0},
  /* 14 */ {2, 3, 4},
  /* 15 */ {2, 3, 0},
  /* 16 */ {2, 0, 0},
  /* 17 */ {2, 1, 0},
  /* 18 */ {2, 1, 8},
  /* 19 */ {1, 2, 0},
  /* 20 */ {1, 0, 0},
  /* 21 */ {1, 8, 0},
  /* 22 */ {8, 1, 2},
  /* 23 */ {8, 1, 0},
  /* 24 */ {8, 0, 0},
  /* 25 */ {8, 7, 0},
  /* 26 */ {8, 7, 6},
  /* 27 */ {7, 8, 0},
  /* 28 */ {7, 0, 0},
  /* 29 */ {7, 6, 0},
  /* 30 */ {6, 7, 8},
  /* 31 */ {6, 7, 0}
};

int evtCanvas (struct pgEvent *evt);
int bttnPush (struct pgEvent *evt);
int interpret_stroke ();
void process_stroke (int hit);
int check_special_char (int ch);
int *get_current_set ();
void change_mode (int mode);
void tap_key (int key);
void hit_key (int key);
void addButtons ();
int load_bitmaps (void);
int which_zone (int x, int y);

pgcontext gc, gcBitmap;
pghandle bmps[4], lock_bmp, ctrl_bmp, alt_bmp, keys_bmp, app;

int sqrs_[62];
int *sqrs = sqrs_ + 31;
static int XOFF = 65, YOFF = 0;
static int zone_ctr_, zone_list_[3];
int input_mode_ = LOWERCASE_MODE;
int input_lock_ = MODE_NORMAL;
int meta_key_ = 0;
int ctrl_key_ = 0;

/*
 * Send a message to the keyboard application
 *
 * msg : pointer to the command to send
 */
void sendMsgToPgboard (struct keyboard_command * cmd)
{
  if (cmd)
    {
      struct pgmemdata data = {cmd, sizeof (struct keyboard_command), 0};
      pghandle kb = pgFindWidget (PG_KEYBOARD_APPNAME);

      if (kb)
	{
	  pgAppMessage (kb, data);
	}
    }
}

int
main (int argc, char **argv)
{
  int i;
  struct keyboard_command pgb_cmd_hide = {htons (PG_KEYBOARD_HIDE)};
  struct keyboard_command pgb_cmd_show = {htons (PG_KEYBOARD_SHOW)};

  for (i = 0; i < 31; i++)
    sqrs[i] = sqrs_[31 - i] = i * i;

  pgInit (argc, argv);

  app = pgRegisterApp (PG_APP_TOOLBAR, "QuickWriting", 0);
  pgSetWidget (PGDEFAULT, PG_WP_SIDE, PG_S_BOTTOM, PG_WP_SIZE, 65, 0);

  sendMsgToPgboard (&pgb_cmd_hide);

  pgNewWidget (PG_WIDGET_CANVAS, PG_DERIVE_INSIDE, 0);
  pgSetWidget (0,
	       PG_WP_TRIGGERMASK, pgGetWidget (0, PG_WP_TRIGGERMASK)
	       | PG_TRIGGER_MOVE, PG_WP_SIDE, PG_S_ALL,
	       0);
  pgBind (PGDEFAULT, PGBIND_ANY, &evtCanvas, NULL);
  gc = pgNewCanvasContext (PGDEFAULT, PGFX_PERSISTENT);

  load_bitmaps ();
  pgBitmap (gc, XOFF, YOFF, 60, 60, bmps[LOWERCASE_MODE]);

  addButtons ();

  pgEventLoop ();

  sendMsgToPgboard (&pgb_cmd_show);

  pgEventPoll ();

  return 0;
}

int
load_bitmaps (void)
{
  bmps[LOWERCASE_MODE] =
    pgNewBitmap (pgFromMemory (lower_case_pbm, sizeof (lower_case_pbm)));
  bmps[UPPERCASE_MODE] =
    pgNewBitmap (pgFromMemory (upper_case_pbm, sizeof (upper_case_pbm)));
  bmps[NUMBER_MODE] = pgNewBitmap (pgFromMemory (numbers_pbm,
						 sizeof (numbers_pbm)));
  bmps[SYMBOL_MODE] = pgNewBitmap (pgFromMemory (symbols_pbm,
						 sizeof (symbols_pbm)));

  lock_bmp = pgNewBitmap (pgFromMemory (lock_pbm, sizeof (lock_pbm)));
  ctrl_bmp = pgNewBitmap (pgFromMemory (ctl_pbm, sizeof (ctl_pbm)));
  alt_bmp = pgNewBitmap (pgFromMemory (alt_pbm, sizeof (alt_pbm)));
  keys_bmp = pgNewBitmap (pgFromMemory (keys_pbm, sizeof (keys_pbm)));

  return 0;
}

void
addButtons ()
{
  /*Replace with Bitmap */
  pgMoveTo (gc, XOFF - 20, 0);
  pgLineTo (gc, XOFF - 20, 60);
  pgLineTo (gc, XOFF - 40, 60);
  pgLineTo (gc, XOFF - 40, 0);
  pgLineTo (gc, XOFF - 20, 0);

  pgMoveTo (gc, XOFF - 60, 20);
  pgLineTo (gc, XOFF - 1, 20);
  pgLineTo (gc, XOFF - 1, 40);
  pgLineTo (gc, XOFF - 60, 40);
  pgLineTo (gc, XOFF - 60, 20);
  pgBitmap (gc, XOFF - 60, 0, 60, 60, keys_bmp);
}

void
checkButtons (int x, int y)
{
  if (x < XOFF - 60)
    return;
  if (x < XOFF - 40)
    {
      if (y > 20 && y < 40)
	hit_key (PGKEY_LEFT);
      return;
    }
  if (x > XOFF - 20)
    {
      if (y > 20 && y < 40)
	hit_key (PGKEY_RIGHT);
      return;
    }
  if (y > 40)
    hit_key (PGKEY_DOWN);
  else if (y > 20)
    pgExitEventLoop ();
  else
    hit_key (PGKEY_UP);
}

int
bttnPush (struct pgEvent *evt)
{
  int i = *(int *) evt->extra;
  switch (i)
    {
    case 0:			/*Done */
      pgExitEventLoop ();
    case 1:
      hit_key (PGKEY_RIGHT);
      break;
    case 2:
      hit_key (PGKEY_UP);
      break;
    case 3:
      hit_key (PGKEY_DOWN);
      break;
    case 4:
      hit_key (PGKEY_LEFT);
      break;
    }
  return 0;
}

int
evtCanvas (struct pgEvent *evt)
{
  int z;
  static int last_zone;

  static int down;

  z = which_zone (evt->e.pntr.x, evt->e.pntr.y);
  switch (evt->type)
    {

    case PG_WE_PNTR_DOWN:
      last_zone = z;
      down = 1;
      if (!z)
	zone_ctr_ = 0;
      if (evt->e.pntr.x < XOFF)
	checkButtons (evt->e.pntr.x, evt->e.pntr.y);
      return 0;
      break;
    case PG_WE_PNTR_UP:
    case PG_WE_PNTR_RELEASE:
      if (z && !zone_ctr_ && z == last_zone)
	tap_key (z);
      down = 0;
      zone_ctr_ = 0;
      return 0;
    case PG_WE_PNTR_MOVE:
      if (!down)
	return 0;		/* How? */
      z = which_zone (evt->e.pntr.x, evt->e.pntr.y);
      if (z != last_zone)
	{
	  if (!z)
	    {
	      process_stroke (interpret_stroke ());
	    }
	  else if (zone_ctr_ < 3)
	    zone_list_[zone_ctr_++] = z;
	  else
	    zone_ctr_ = 0;
	  last_zone = z;
	}
      break;
    }

  return 0;
}

int
which_zone (int x, int y)
{
  x -= XOFF + 30;
  y -= YOFF + 30;
  if (x > 30 || y > 30 || x < -30 || y < -30)
    return 0;

  if (sqrs[x] + sqrs[y] < 225)
    return 0;
  else if (x > 0)
    {
      if (y > 0)
	{
	  if (3 * y < x)
	    return 3;
	  else if (3 * x < y)
	    return 1;
	  else
	    return 2;
	}
      else
	{
	  if (-3 * y < x)
	    return 3;
	  else if (3 * x < -y)
	    return 5;
	  else
	    return 4;
	}
    }
  else
    {
      if (y > 0)
	{
	  if (3 * y < -x)
	    return 7;
	  else if (-3 * x < y)
	    return 1;
	  else
	    return 8;
	}
      else
	{
	  if (3 * y > x)
	    return 7;
	  else if (3 * x > y)
	    return 5;
	  else
	    return 6;
	}
    }
  return 0;
}

int
interpret_stroke ()
{
  int ret = -1;
  int n;

  /* Look for a match in the set of zones. */
  memset (zone_list_ + zone_ctr_, 0,
	  sizeof (zone_list_[0]) * (3 - zone_ctr_));
  for (n = 0; n < 32; n++)
    {
      if (zone_list_[0] == zone_data[n][0] &&
	  zone_list_[1] == zone_data[n][1] &&
	  zone_list_[2] == zone_data[n][2])
	{
	  ret = n;
	  break;
	}
    }

  /* Reset the zone list */
  zone_ctr_ = 0;
  memset (zone_list_, 0, sizeof (zone_list_[0]) * 3);

  return ret;
}

void
process_stroke (int hit)
{
  int *l = get_current_set ();
  if (hit == -1)
    return;

  if (!check_special_char (l[hit]))
    {
      hit_key (l[hit]);

      /* Check input modes, see if we need to reset */
      if (input_lock_ == MODE_ONCE)
	{
	  input_lock_ = MODE_NORMAL;
	  input_mode_ = LOWERCASE_MODE;
	  pgBitmap (gc, XOFF, YOFF, 60, 60, bmps[LOWERCASE_MODE]);
	  pgContextUpdate (gc);
	}
    }

}

/**
 * Check the input character to see if it should influence our
 * input mode.
 * \return 0 if the char didn't affect mode (so should be output)
 * or nonzero otherwise.
 */
int
check_special_char (int ch)
{
  int ret = 0;

  switch (ch)
    {
    case SHIFT_CHAR:
      {
	change_mode (UPPERCASE_MODE);
	ret = 1;
	break;
      }
    case NUMBER_CHAR:
      {
	change_mode (NUMBER_MODE);
	ret = 1;
	break;
      }
    case SYMBOL_CHAR:
      {
	change_mode (SYMBOL_MODE);
	ret = 1;
	break;
      }
    case UNSHIFT_CHAR:
      {
	change_mode (LOWERCASE_MODE);
	ret = 1;
	break;
      }
    }

  return ret;
}

/**
 * Change the input mode states, tracking multiple hits
 * against a mode button to lock/unlock the mode state.
 */
void
change_mode (int mode)
{
  if (input_mode_ != mode)
    {
      input_mode_ = mode;
      input_lock_ = MODE_ONCE;
    }
  else
    {
      input_lock_++;

      /* Rotated through the set of locks, so reset. */
      if (input_lock_ > MODE_LOCK)
	{
	  input_mode_ = LOWERCASE_MODE;
	  input_lock_ = MODE_NORMAL;
	}
    }

  if (input_lock_ == MODE_LOCK)
    {
      pgBitmap (gc, XOFF + 26, YOFF + 26, 8, 8, lock_bmp);
    }
  else
    {
      pgBitmap (gc, XOFF, YOFF, 60, 60, bmps[input_mode_]);
    }
  pgContextUpdate (gc);
}

/**
 * Retrieve the current character set for the current input
 * mode.
 */
int *
get_current_set ()
{
  return (char_sets[input_mode_]);
}

void
tap_key (int key)
{
  switch (tap_keys[key])
    {
    case PGKEY_LALT:
      meta_key_ = !meta_key_;
      if (meta_key_)
	pgBitmap (gc, XOFF + 42, YOFF + 48, 11, 5, alt_bmp);
      else
	pgBitmap (gc, XOFF, YOFF, 60, 60, bmps[input_mode_]);
      pgContextUpdate (gc);
      break;
    case PGKEY_LCTRL:
      ctrl_key_ = !ctrl_key_;
      if (ctrl_key_)
	pgBitmap (gc, XOFF + 7, YOFF + 48, 11, 5, ctrl_bmp);
      else
	pgBitmap (gc, XOFF, YOFF, 60, 60, bmps[input_mode_]);
      pgContextUpdate (gc);
      break;
    default:
      hit_key (tap_keys[key]);
      break;
    }
}

void pqwSendKeyInput (u32 type, u32 key, u32 mods)
{
  static union pg_client_trigger trig;
  trig.content.type = type;
  trig.content.u.kbd.key = key;
  trig.content.u.kbd.mods = mods;
  pgInFilterSend(&trig);
}

void
hit_key (int key)
{
  unsigned short mods = 0;

  if (ctrl_key_)
    mods |= PGMOD_LCTRL;
  if (meta_key_)
    mods |= PGMOD_LALT;
  if (key >= 'A' && key <= 'Z')
    {
      mods |= PGMOD_SHIFT;
/*		key += 'a' - 'A';*/
    }
  pqwSendKeyInput (PG_TRIGGER_KEYDOWN, key, mods);
  if (!ctrl_key_ && !meta_key_ && key < 128)
    pqwSendKeyInput (PG_TRIGGER_CHAR, key, mods);
  pqwSendKeyInput (PG_TRIGGER_KEYUP, key, mods);

  if (ctrl_key_ || meta_key_)
    {
      pgBitmap (gc, XOFF, YOFF, 60, 60, bmps[input_mode_]);
      pgContextUpdate (gc);
    }
  if (ctrl_key_)
    ctrl_key_ = 0;
  if (meta_key_)
    meta_key_ = 0;
}
