/*
 *   "$Id: gx11.c,v 1.28 2003/02/21 17:26:07 crowbert Exp $";
 */
/*
  Crossfire client, a client program for the crossfire program.

  Copyright (C) 2001 Mark Wedel & Crossfire Development Team

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA

  The author can be reached via e-mail to crossfire-devel@real-time.com
*/

#include <picogui.h>
/* always include our local headers after the system headers are included */
#include "client.h"
/*#include "clientbmap.h"*/
#include "item.h"

bool is_textmode = FALSE;
int map_widget_size;
pghandle info_widget, map_widget, title_label,
  hp_indicator, sp_indicator, gr_indicator, food_indicator,
  hp_label, sp_label, gr_label, food_label,
  resistances_tb, stats_pane, stats_tb,
  default_look_title, temp_look_title,
  weight_label, look_label;
pgcontext map_context;


#define MAX_LIST_ITEMS		1023
struct list_item
{
  pghandle item, weight;
} inv_items[MAX_LIST_ITEMS + 1], look_items[MAX_LIST_ITEMS + 1];


struct Map the_map;
PlayerPosition pl_pos;
item *looking_at = NULL;
uint32 weight_limit;

static struct pgmemdata colorcmd[] = {
  /* these have to be all of the same len (13), so they follow a strict format:
     0;01;bg;fg or 0;22;bg;fg
  */
  {"\033[0;05;22;47;30m", 16, 0},                /* 0 - Black */
  {"\033[0;25;01;40;37m", 16, 0},                /* 1 - White  */
  {"\033[0;05;22;47;30m", 16, 0},                /* 2 - Navy  */
  {"\033[0;05;22;47;31m", 16, 0},                /* 3 - Red  */
  {"\033[0;05;01;47;31m", 16, 0},                /* 4 - Orange  */
  {"\033[0;05;22;47;30m", 16, 0},                /* 5 - DodgerBlue  */
  {"\033[0;05;22;47;30m", 16, 0},                /* 6 - DarkOrange2  */
  {"\033[0;05;01;47;32m", 16, 0},                /* 7 - SeaGreen  */
  {"\033[0;05;22;47;30m", 16, 0},                /* 8 - DarkSeaGreen  */        /* Used for window background color */
  {"\033[0;05;22;47;30m", 16, 0},                /* 9 - Grey50  */
  {"\033[0;05;22;47;30m", 16, 0},                /* 10 - Sienna */
  {"\033[0;05;22;47;30m", 16, 0},                /* 11 - Gold */
  {"\033[0;05;22;47;30m", 16, 0},                /* 12 - Khaki */
};
#define COLORCODE_MAX 12
static struct pgmemdata pg_crlf = {"\n", 1, 0};
static struct pgmemdata command_colorcmd = {"\033[0;22;40;32m", 13, 0};


#define DEFAULT_IMAGE_SIZE	32

int image_size=DEFAULT_IMAGE_SIZE;
#define MAXIMAGENUM 10000
struct image_data
{
  u16 width;
  u16 height;
  pghandle handle;
} images[MAXIMAGENUM];

void draw_info (const char *str, int color);


void menu_clear ()
{
  fprintf (stderr, ">>> called void menu_clear ()\n");
}


void save_winpos ()
{
  fprintf (stderr, ">>> called void save_winpos ()\n");
}


void command_show (char *params)
{
  fprintf (stderr, ">>> called void command_show ()\n");
}


void x_set_echo () {
  /* does nothing */
}


void
draw_prompt (const char *str)
{
  draw_info (str, NDI_BLACK);
}


/******************************************************************************
 *
 * Functions dealing with inventory and look lists
 *
 *****************************************************************************/


void open_container (item *op) 
{
  looking_at = op;
  looking_at->inv_updated = 1;
  temp_look_title = pgNewString (op->d_name);
  pgSetWidget (look_label,
	       PG_WP_TEXT, temp_look_title,
	       0);
}


void close_container (item *op) 
{
  if (looking_at == op)
    {
      client_send_apply (looking_at->tag);
      looking_at = cpl.below;
      looking_at->inv_updated = 1;
      if (temp_look_title)
	pgDelete (temp_look_title);
      temp_look_title = 0;
      pgSetWidget (look_label,
		   PG_WP_TEXT, default_look_title,
		   0);
    }
}


void set_weight_limit (uint32 wlim)
{
  weight_limit = wlim / 1000;
}


int item_click_handler (struct pgEvent *evt)
{
  item *it = evt->extra;
  printf ("Yay, a click on `%s' with buttons %d, %d (%x)!\n",
	  it->d_name, evt->e.pntr.btn, evt->e.pntr.chbtn, evt->e.param);
}


void draw_list (item *op, struct list_item *items)
{
  item *i;
  int index = 1;
  char buff[MAX_BUF];
  pghandle old_s;
  s32 derive = PG_DERIVE_INSIDE;

  for (i = op->inv; i ; i=i->next, index++)
    {
      if (index == MAX_LIST_ITEMS)
	{
	  items[index].item = pgNewWidget (PG_WIDGET_LABEL, PG_DERIVE_AFTER, items[index-1].item);
	  pgSetWidget (items[index].item,
		       PG_WP_TEXT, pgNewString (" (and more)"),
		       0);
	  break;
	}

      if (!items[index].item)
	{
	  items[index].item = pgNewWidget (PG_WIDGET_MENUITEM, derive, items[index-1].item);
	  pgSetWidget (items[index].item,
		       PG_WP_ALIGN, PG_A_LEFT,
		       PG_WP_EXTDEVENTS, PG_EXEV_PNTR_UP,
		       0);
	  pgBind (items[index].item, PG_WE_PNTR_UP, &item_click_handler, i);
	  items[index].weight = pgNewWidget (PG_WIDGET_LABEL, PG_DERIVE_INSIDE, PGDEFAULT);
	  pgSetWidget (items[index].weight,
		       PG_WP_ALIGN, PG_A_RIGHT,
		       PG_WP_SIDE, PG_S_RIGHT,
		       0);
	}

      fprintf (stderr, "drawing item: %s (%d - %04x)\n",
	       i->d_name, i->face, images[i->face].handle);
      snprintf (buff, MAX_BUF, "%s %s", i->d_name, i->flags);
      buff[MAX_BUF-1] = '\0';
      old_s = pgGetWidget (items[index].item, PG_WP_TEXT);
      pgSetWidget (items[index].item,
		   PG_WP_TEXT, pgNewString (buff),
		   PG_WP_IMAGE, images[i->face].handle,
		   0);
      pgDelete (old_s);

      if (i->weight < 0)
	buff[0] = '\0';
      else
	{
	  snprintf (buff, MAX_BUF, "%6.1f" ,i->nrof * i->weight);
	  buff[MAX_BUF-1] = '\0';
	}
      old_s = pgGetWidget (items[index].weight, PG_WP_TEXT);
      pgSetWidget (items[index].weight,
		   PG_WP_TEXT, pgNewString (buff),
		   0);
      pgDelete (old_s);

      derive = PG_DERIVE_AFTER;
    }

  /* delete menuitems that became unused due to the list shrinking */
  while (index <= MAX_LIST_ITEMS && items[index].item)
    {
      pgDelete (pgGetWidget (items[index].item, PG_WP_TEXT));
      pgDelete (items[index].item);
      items[index].item = 0;

      pgDelete (pgGetWidget (items[index].weight, PG_WP_TEXT));
      pgDelete (items[index].weight);
      items[index].weight = 0;

      index++;
    }

  op->inv_updated = 0;
  pgSubUpdate (items[0].item);
}


void draw_lists ()
{
  static uint32 recorded_weight_limit = 0;
  char buff[MAX_BUF];
  pghandle old_s;

  if (cpl.ob->inv_updated || (recorded_weight_limit != weight_limit))
    {
      recorded_weight_limit = weight_limit;

      snprintf (buff, MAX_BUF, "Carrying:  %6.1f/%d", cpl.ob->weight, weight_limit);
      buff[MAX_BUF-1] = '\0';
      old_s = pgGetWidget (weight_label, PG_WP_TEXT);
      pgSetWidget (weight_label,
		   PG_WP_TEXT, pgNewString (buff),
		   0);
      pgDelete (old_s);

      draw_list (cpl.ob, inv_items);
    }
  if (looking_at->inv_updated)
    draw_list (looking_at, look_items);
}


/******************************************************************************
 *
 * The functions dealing with the info window follow
 *
 *****************************************************************************/


/* draw_info adds a line to the info window.
 */

void draw_info (const char *str, int color)
{
  color &= NDI_COLOR_MASK;
  pgWriteData (info_widget, colorcmd[color]);
  pgWriteData (info_widget, pgFromMemory (str, strlen (str)));
  pgWriteData (info_widget, pg_crlf);
  pgSubUpdate (info_widget);
}


void draw_color_info (int colr, const char *buf){
  if (use_config[CONFIG_COLORTXT]){
    draw_info (buf,colr);
  }
  else {
    draw_info ("==========================================",NDI_BLACK);
    draw_info (buf,NDI_BLACK);
    draw_info ("==========================================",NDI_BLACK);
  }
}



/***********************************************************************
 *
 * Stats window functions follow
 *
 ***********************************************************************/


/* This draws the stats window.  If redraw is true, it means
 * we need to redraw the entire thing, and not just do an
 * updated.
 */

void draw_stats (int redraw)
{
  int i, l;
  float weap_sp;
  char buff[MAX_BUF];
  pghandle old_s;

  /* title: player title and level */
  snprintf (buff, MAX_BUF, "Crossfire %.240s (%d)", cpl.title, cpl.stats.level);
  buff[MAX_BUF-1] = '\0';
  old_s = pgGetWidget (title_label, PG_WP_TEXT);
  pgSetWidget (title_label,
	       PG_WP_TEXT, pgNewString (buff),
	       0);
  pgDelete (old_s);
  pgSubUpdate (title_label);

  /* stats */
  sprintf (buff,"S:%d  D:%d  Co:%d  I:%d  W:%d  P:%d  Ch:%d\nXP:%d(%d)  ",
	   cpl.stats.Str, cpl.stats.Dex, cpl.stats.Con, cpl.stats.Int,
	   cpl.stats.Wis, cpl.stats.Pow, cpl.stats.Cha, cpl.stats.exp, cpl.stats.level);
  old_s = pgGetWidget (stats_tb, PG_WP_TEXT);
  pgSetWidget (stats_tb,
	       PG_WP_READONLY, 0,
	       PG_WP_INSERTMODE, PG_INSERT_OVERWRITE,
	       PG_WP_TEXT, pgNewString (buff),
	       PG_WP_INSERTMODE, PG_INSERT_APPEND,
	       0);
  pgDelete (old_s);

  for (i=0; i<MAX_SKILL; i++)
    {
      l = sprintf (buff,"%s:%d(%d)  ", skill_names[i],
		   cpl.stats.skill_exp[i], cpl.stats.skill_level[i]);
      pgWriteData (stats_tb, pgFromMemory (buff, l));
    }
  weap_sp = (float) cpl.stats.speed / ((float)cpl.stats.weapon_sp);
  l = sprintf (buff, "\n%s\nWc:%d  Dam:%d  Ac:%d  Speed:%3.2f (weapon:%1.2f)\n",
	       cpl.range, cpl.stats.wc, cpl.stats.dam, cpl.stats.ac,
	       (float)cpl.stats.speed/FLOAT_MULTF, weap_sp);
  pgWriteData (stats_tb, pgFromMemory (buff, l));
  pgSetWidget (stats_tb,
	       PG_WP_READONLY, 1,
	       0);
  pgSubUpdate (stats_tb);

  /* vitals */
  sprintf (buff,"HP: %d/%d", cpl.stats.hp, cpl.stats.maxhp);
  old_s = pgGetWidget (hp_label, PG_WP_TEXT);
  pgSetWidget (hp_label,
	       PG_WP_TEXT, pgNewString (buff),
	       0);
  pgDelete (old_s);
  pgSubUpdate (hp_label);

  sprintf (buff,"SP: %d/%d", cpl.stats.sp, cpl.stats.maxsp);
  old_s = pgGetWidget (sp_label, PG_WP_TEXT);
  pgSetWidget (sp_label,
	       PG_WP_TEXT, pgNewString (buff),
	       0);
  pgDelete (old_s);
  pgSubUpdate (sp_label);

  sprintf (buff,"Gr: %d/%d", cpl.stats.grace, cpl.stats.maxgrace);
  old_s = pgGetWidget (gr_label, PG_WP_TEXT);
  pgSetWidget (gr_label,
	       PG_WP_TEXT, pgNewString (buff),
	       0);
  pgDelete (old_s);
  pgSubUpdate (gr_label);

  sprintf (buff,"Food: %d", cpl.stats.food);
  old_s = pgGetWidget (food_label, PG_WP_TEXT);
  pgSetWidget (food_label,
	       PG_WP_TEXT, pgNewString (buff),
	       0);
  pgDelete (old_s);
  pgSubUpdate (food_label);
}


/***********************************************************************
 *
 * Handles the message window
 *
 ***********************************************************************/


/* This updates the status bars.  If redraw, then redraw them
 * even if they have not changed
 */

void draw_indicator (pghandle indicator, s32 value)
{
  s32 color;

  if (value <= 0)
    {
      color = 0xff8080;
      value = 100;
    }
  else if (value > 100)
    {
      color = 0x000080;
      value = 100;
    }
  else
    {
      color = (value * 0xff00 / 100) & 0xff00;
      color |= 0xff0000 - ((value * 0xff0000 / 100) & 0xff0000);
    }
  pgSetWidget (indicator,
	       PG_WP_VALUE, value,
	       PG_WP_COLOR, color,
	       0);
  pgSubUpdate (indicator);
}

void draw_message_window (int redraw)
{
  int i, l;
  char buff[MAX_BUF];
  pghandle old_s;

  /* indicators */
  draw_indicator (hp_indicator, (cpl.stats.hp * 100) / cpl.stats.maxhp);
  draw_indicator (sp_indicator, (cpl.stats.sp * 100) / cpl.stats.maxsp);
  draw_indicator (gr_indicator, (cpl.stats.grace * 100) / cpl.stats.maxgrace);
  draw_indicator (food_indicator, cpl.stats.food / 10);

  /* the resistances */
  old_s = pgGetWidget (resistances_tb, PG_WP_TEXT);
  pgSetWidget (resistances_tb,
	       PG_WP_READONLY, 0,
	       PG_WP_INSERTMODE, PG_INSERT_OVERWRITE,
	       PG_WP_TEXT, pgNewString (" "),
	       PG_WP_INSERTMODE, PG_INSERT_APPEND,
	       0);
  pgDelete (old_s);
  for (i=0; i<NUM_RESISTS; i++)
    {
      if (cpl.stats.resists[i]) {
	l = sprintf (buff, "%.10s %+4d   ",
		     resists_name[i], cpl.stats.resists[i]);
	pgWriteData (resistances_tb, pgFromMemory (buff, l));
      }
    }
  pgWriteData (resistances_tb, pg_crlf);
  pgSetWidget (resistances_tb,
	       PG_WP_READONLY, 1,
	       0);
  pgSubUpdate (resistances_tb);
}


/******************************************************************************
 *
 * Keyboard functions
 *
 *****************************************************************************/


void bind_key (char *params)
{
  fprintf (stderr, ">>> called void bind_key ()\n");
}


void unbind_key (char *params)
{
  fprintf (stderr, ">>> called void unbind_key ()\n");
}


void pg_input_string (union pg_client_trigger *trig, void action (char*))
{
  static char *text = NULL;
  static char buff[2] = {0, 0};
  static size_t tsize = 0, tlen = 0;
  static struct pgmemdata backspace = {"\b \b", 3, 0};

  if (trig->content.type != PG_TRIGGER_CHAR)
    return;

  if (trig->content.u.kbd.key == PGKEY_RETURN)
    {
      if (tlen)
	{
	  text[tlen] = '\0';
	  action (text);
	  if (!cpl.no_echo)
	    pgWriteData (info_widget, pg_crlf);
	}
      else
	action ("");
      if (tsize)
	{
	  free (text);
	  tsize = tlen = 0;
	  text = NULL;
	}
      cpl.input_state = Playing;
      return;
    }
  if (trig->content.u.kbd.key == PGKEY_BACKSPACE)
    {
      if (tlen)
	{
	  tlen--;
	  if (!cpl.no_echo)
	    pgWriteData (info_widget, backspace);
	}
      return;
    }
  if (tlen + 1 >= tsize)
    {
      tsize = tsize ? tsize << 1 : 64;
      text = realloc (text, tsize);
      if (text==NULL)
	{
	  fprintf (stderr, "out of memory\n");
	  exit (13);
	}
    }
  if (!cpl.no_echo)
    {
      buff[0] = trig->content.u.kbd.key & 0x7f;
      pgWriteData (info_widget, pgFromMemory (buff, 1));
    }
  text[tlen++] = trig->content.u.kbd.key;
}


int keyboard_handler (struct pgEvent *evt)
{
  static char *text = NULL;
  static char buff[2] = {0, 0};
  static size_t tsize = 0, tlen = 0;
  static int is_shutdown = 0;
  union pg_client_trigger *trig = evt->e.data.trigger;

  if (csocket.fd==-1)
    {
      fprintf (stderr, "server hang up on us!\n");
      if (!is_shutdown)
	{
	  is_shutdown = 1;
	  draw_info ("\nServer hang up\n\n", NDI_RED);
  	}
      else if (trig->content.u.kbd.key == PGKEY_q)
	{
	  pgExitEventLoop();
	  return 1;
	}
      return 0;
    }


  switch (cpl.input_state) {

  case Playing:
    if (trig->content.u.kbd.key == PGKEY_QUOTE)
      {
	pgWriteData (info_widget, command_colorcmd);
	cpl.input_state = Command_Mode;
	cpl.no_echo=FALSE;
      }
    /* de-translate uppercase chars */
    if (trig->content.u.kbd.key > PGKEY_AT && trig->content.u.kbd.key < PGKEY_LEFTBRACKET)
      {
	trig->content.u.kbd.key += 32;
	trig->content.u.kbd.mods |= PGMOD_SHIFT;
      }
    printf("K %s (%d), mods %x\n", pgKeyName (trig->content.u.kbd.key), trig->content.u.kbd.key,
	   trig->content.u.kbd.mods);
    break;

  case Reply_One:
    if (trig->content.type != PG_TRIGGER_CHAR)
      return;
    buff[0] = trig->content.u.kbd.key & 0x7f;
    send_reply (buff);
    cpl.input_state = Playing;
    break;

  case Reply_Many:
    pg_input_string (trig, send_reply);
    break;

  case Configure_Keys:
    break;

  case Command_Mode:
    pg_input_string (trig, extended_command);
    break;

  case Metaserver_Select:
    break;

  default:
    fprintf (stderr,"Unknown input state: %d\n", cpl.input_state);
  }
  return 0;
}


/******************************************************************************
 *
 * Image-related functions
 *
 *****************************************************************************/


int create_and_rescale_image_from_data (Cache_Entry *ce, int pixmap_num, uint8 *rgba_data, int width, int height)
{
  pghandle image = (pghandle) rgba_data;

  fprintf (stderr, ">>> called create_and_rescale_image_from_data (num = %d, %dx%d)\n",
	   pixmap_num, width, height);

  if (pixmap_num >= MAXIMAGENUM)
    {
      fprintf (stderr, "Ooops!  Images table is too small - tried to store bitmap %d out of %d possible entries\n",
	       pixmap_num, MAXIMAGENUM);
      exit (1);
    }

  if (is_textmode)
    {
      static struct pgmemdata unknown = {"AA\0\xf0?", 5};
      static pghandle unknown_i = 0;

      if (!unknown_i)
	{
	  unknown_i = pgNewBitmap (unknown);
	  fprintf (stderr, ">>> created unknown_i: %04x\n", unknown_i);
	}

      pgDelete (image);
      image = unknown_i;
    }

  images[pixmap_num].width = width;
  images[pixmap_num].height = height;
  images[pixmap_num].handle = image;

  return 0;
}


uint8 *png_to_data (uint8 *data, int len, uint32 *width, uint32 *height)
{
  pghandle image;

  image = pgNewBitmap (pgFromMemory (data, len));
  if (!image)
    /* loading error */
    return NULL;
  pgSizeBitmap (width, height, image);

  return (uint8*) image;
}


/* This functions associates the image_data in the cache entry
 * with the specific pixmap number.  Returns 0 on success, -1
 * on failure.  Currently, there is no failure condition, but
 * there is the potential that in the future, we want to more
 * closely look at the data and if it isn't valid, return
 * the failure code.
 */
int associate_cache_entry (Cache_Entry *ce, int pixnum)
{
  fprintf (stderr, ">>> called associate_cache_entry ()\n");
  /* no-op - we don't yet support caching */
  return 0;
}


void get_map_image_size (int face, uint8 *w, uint8 *h)
{
  /* We want to calculate the number of spaces this image
   * uses it.  By adding the image size but substracting one,	
   * we cover the cases where the image size is not an even
   * increment.  EG, if the map_image_size is 32, and an image
   * is 33 wide, we want that to register as two spaces.  By
   * adding 31, that works out.
   */
  if (face == 0) {
    *w = 1;
    *h = 1;
  } else {
    *w = (images[face].width + image_size - 1)/ image_size;
    *h = (images[face].height + image_size - 1)/ image_size;
  }
  fprintf (stderr, ">>> called void get_map_image_size (%d, %d, %d)\n", face, *w, *h);
}


/******************************************************************************
 *
 * Map-drawing stuff
 *
 *****************************************************************************/


/*
 * Takes three args, first is a return value that is a pointer
 * we should put map info into. Next two are map dimensions.
 * This function supports non rectangular maps but the client
 * pretty much doesn't. The caller is responsible for freeing
 * the data. I have the caller pass in a map struct instead of
 * returning a pointer because I didn't want to change all the
 * the_map.cells to the_map->cells...
 * The returned map memory is zero'ed.
 */
void allocate_map ( struct Map* new_map, int ax, int ay)
{
  int i= 0;

  fprintf (stderr, ">>> allocating map\n");

  if ( new_map == NULL)
    return;

  if ( ax < 1 || ay < 1) {
    new_map->cells= NULL;
    fprintf (stderr, "null map requested!\n");
    return;
  }

  new_map->cells= (struct MapCell**)calloc ( sizeof ( struct MapCell*) * ay
					     + sizeof ( struct MapCell) * ax * ay, 1);

  if ( new_map->cells == NULL)
    {
      fprintf (stderr, "could not allocate map cells!\n");
      return;
    }

  /* Skip past the first row of pointers to rows and assign the start of
   * the actual map data
   */
  new_map->cells[0]= (struct MapCell*) ((char*)new_map->cells + 
					(sizeof ( struct MapCell*) * ay));

  /* Finish assigning the beginning of each row relative to the first row
   * assigned above
   */
  for ( i= 0; i < ay; i++)  {
    new_map->cells[i]= new_map->cells[0] + ( i * ax);
  }
  new_map->x= ax;
  new_map->y= ay;

  fprintf (stderr, "map allocated\n");
  return;
}


/*
 * Clears out all the cells in the current view (which is 
 * the whole map if not using fog_of_war, and request
 * a map update from the server 
 */
void reset_map ()
{
  int x= 0;
  int y= 0;

  pl_pos.x= the_map.x/2;
  pl_pos.y= the_map.y/2;
  memset ( the_map.cells[0], 0, 
	   sizeof ( struct MapCell) * the_map.x * the_map.y);
  for ( x= pl_pos.x; x < (pl_pos.x + use_config[CONFIG_MAPWIDTH]); x++) 
    {
      for ( y= pl_pos.y; y < (pl_pos.y + use_config[CONFIG_MAPHEIGHT]); y++)
	{
	  the_map.cells[x][y].need_update= 1;
	}
    }
  cs_print_string (csocket.fd, "mapredraw");
  return;
}


/* Do the map drawing */
void pg_text_draw_map (int redraw)
{
  int mx,my, layer,x,y, src_x, src_y;
  struct MapCell *cell;

  for( x= 0; x < use_config[CONFIG_MAPWIDTH] * 2; x+=2)
    {
      for(y = 0; y < use_config[CONFIG_MAPHEIGHT]; y++)
	{
	  mx = (x/2) + pl_pos.x;
	  my = y + pl_pos.y;
	  cell = &the_map.cells[mx][my];

	  /* Don't need to touch this space */
	  if (!redraw && !cell->need_update) continue;

	  fprintf (stderr, ">>> textmode: drawing %2d, %2d: [%4d (%04x), %4d (%04x), %4d (%04x)]",
		   x, y,
		   cell->heads[0].face, images[cell->heads[0].face].handle,
		   cell->heads[1].face, images[cell->heads[1].face].handle,
		   cell->heads[2].face, images[cell->heads[2].face].handle);
	  if (cell->tails[0].face || cell->tails[1].face || cell->tails[2].face)
	    fprintf (stderr, "  tails: [%4d (%04x), %4d (%04x), %4d (%04x)]",
		     cell->tails[0].face, images[cell->tails[0].face].handle,
		     cell->tails[1].face, images[cell->tails[1].face].handle,
		     cell->tails[2].face, images[cell->tails[2].face].handle);

	  if (cell->heads[0].face)
	    {
	      fprintf (stderr, "  - floor (%04x)", images[cell->heads[0].face].handle);
	      pgBitmap (map_context, x+1, y, 1, 1, images[cell->heads[0].face].handle);
	    }
	  else
	    {
	      /* nothing to draw, move along */
	      pgSetColor (map_context, 0);
	      pgRect (map_context, x, y, 2, 1);
	    }

	  fprintf (stderr, "\n");
	}
    }
}


void pg_graphic_draw_map (int redraw)
{
  int mx,my, layer,x,y, src_x, src_y;
  struct MapCell *cell;
  struct image_data *i;

  for( x= 0; x < use_config[CONFIG_MAPWIDTH]; x++)
    {
      for(y = 0; y < use_config[CONFIG_MAPHEIGHT]; y++)
	{
	  mx = x + pl_pos.x;
	  my = y + pl_pos.y;
	  cell = &the_map.cells[mx][my];

	  /* Don't need to touch this space */
	  if (!redraw && !cell->need_update) continue;

	  /* First, we need to black out this space. */
	  pgSetColor (map_context, 0);
	  pgRect (map_context,
		    x * image_size, y * image_size, image_size, image_size);

	  fprintf (stderr, ">>> graphics: drawing %2d, %2d: [%4d (%04x), %4d (%04x), %4d (%04x)]",
		   x, y,
		   cell->heads[0].face, images[cell->heads[0].face].handle,
		   cell->heads[1].face, images[cell->heads[1].face].handle,
		   cell->heads[2].face, images[cell->heads[2].face].handle);
	  if (cell->tails[0].face || cell->tails[1].face || cell->tails[2].face)
	    fprintf (stderr, "  tails: [%4d (%04x), %4d (%04x), %4d (%04x)]",
		     cell->tails[0].face, images[cell->tails[0].face].handle,
		     cell->tails[1].face, images[cell->tails[1].face].handle,
		     cell->tails[2].face, images[cell->tails[2].face].handle);

	  /* actually, we don't do tails - no need, until we start supporting
	   * shading and fogging */
	  for (layer=0; layer<MAXLAYERS; layer++)
	    if (cell->heads[layer].face)
	      {
		i = &images[cell->heads[layer].face];
		pgBitmap (map_context, x * image_size, y * image_size,
			  i->width, i->height, i->handle);
	      }

	  fprintf (stderr, "\n");
	}
    }
}


void display_map_doneupdate (int redraw)
{
  fprintf (stderr, ">>> called void display_map_doneupdate ()\n");

  if (is_textmode)
    pg_text_draw_map (redraw);
  else
    pg_graphic_draw_map (redraw);

  pgSubUpdate (map_widget);
}


void display_map_newmap ()
{
  fprintf (stderr, ">>> called void display_map_newmap ()\n");
  reset_map ();
}


void display_mapscroll (int dx,int dy)
{
  fprintf (stderr, ">>> called void display_mapscroll ()\n");
  pl_pos.x+= dx;
  pl_pos.y+= dy;
  display_map_doneupdate (1);
}


void display_map_startupdate ()
{
  fprintf (stderr, ">>> called void display_map_startupdate ()\n");
}


void resize_map_window (int x, int y)
{
  fprintf (stderr, ">>> called void resize_map_window ()\n");
}


/******************************************************************************
 *
 * Sound code (currently no-op)
 *
 *****************************************************************************/


void SoundCmd (unsigned char *data,  int len)
{
  fprintf (stderr, ">>> called void SoundCmd ()\n");
}


/******************************************************************************
 *
 * Here are the old Xutil commands needed.
 *
 *****************************************************************************/


/* This function draws the magic map in the game window.  I guess if
 * we wanted to get clever, we could open up some other window or
 * something.
 *
 * A lot of this code was taken from server/xio.c  But being all
 * the map data has been figured, it tends to be much simpler.
 */
void draw_magic_map ()
{
  fprintf (stderr, ">>> called void draw_magic_map ()\n");
}


/******************************************************************************
 *
 * The functions dealing with startup and shutdown follow
 *
 *****************************************************************************/


void set_scroll (char *s)
{
  fprintf (stderr, ">>> called void set_scroll ()\n");
}


void set_autorepeat (char *s)
{
  fprintf (stderr, ">>> called void set_autorepeat ()\n");
}


void set_show_icon (char *s)
{
  fprintf (stderr, ">>> called void set_show_icon ()\n");
}


void set_show_weight (char *s)
{
  fprintf (stderr, ">>> called void set_show_weight ()\n");
}


void set_map_darkness (int x, int y, uint8 darkness)
{
  fprintf (stderr, ">>> called void set_map_darkness ()\n");
}


int get_info_width ()
{
  return 50;
}


/* This function draws a little status bar showing where we our
 * in terms of downloading all the image data.
 * start is the start value just sent to the server, end is the end
 * value.  total is the total number of images.
 * A few hacks:
 * If start is 1, this is the first batch, so it means we need to
 * create the appropriate status window.
 * if start = end = total, it means were finished, so destroy
 * the gui element.
 */
void image_update_download_status (int start, int end, int total)
{
  fprintf (stderr, ">>> called void image_update_download_status ()\n");
}


void load_defaults ()
{
  char path[MAX_BUF],inbuf[MAX_BUF],*cp;
  FILE *fp;
  int i, val;

  /* Copy over the want values to use values now */
  for (i=0; i<CONFIG_NUMS; i++) {
    use_config[i] = want_config[i];
  }

  sprintf (path,"%s/.crossfire/pgdefaults", getenv ("HOME"));
  if ((fp=fopen (path,"r")) != NULL)
    {
      while (fgets (inbuf, MAX_BUF-1, fp)) {
	inbuf[MAX_BUF-1]='\0';
	inbuf[strlen (inbuf)-1]='\0';	/* kill newline */

	if (inbuf[0]=='#') continue;
	/* IF no colon, then we certainly don't have a real value, so just skip */
	if (! (cp=strchr (inbuf,':'))) continue;
	*cp='\0';
	cp+=2;	    /* colon, space, then value */

	val = -1;
	if (isdigit (*cp)) val=atoi (cp);
	else if (!strcmp (cp,"True")) val = TRUE;
	else if (!strcmp (cp,"False")) val = FALSE;

	for (i=1; i<CONFIG_NUMS; i++) {
	  if (!strcmp (config_names[i], inbuf)) {
	    if (val == -1) {
	      fprintf (stderr,"Invalid value/line: %s: %s\n", inbuf, cp);
	    } else {
	      want_config[i] = val;
	    }
	    break;	/* Found a match - won't find another */
	  }
	}
	/* We found a match in the loop above, so no need to do anything more */
	if (i < CONFIG_NUMS) continue;

	/* Legacy - now use the map_width and map_height values
	 * Don't do sanity checking - that will be done below
	 */
	if (!strcmp (inbuf,"mapsize")) {
	  if (sscanf (cp,"%hdx%hd", &want_config[CONFIG_MAPWIDTH], &want_config[CONFIG_MAPHEIGHT])!=2) {
	    fprintf (stderr,"Malformed mapsize option in gdefaults.  Ignoring\n");
	  }
	}
	else if (!strcmp (inbuf, "server")) {
	  server = strdup_local (cp);	/* memory leak ! */
	  continue;
	}
	else if (!strcmp (inbuf, "nopopups")) {
	  /* Changed name from nopopups to popups, so inverse value */
	  want_config[CONFIG_POPUPS] = !val;
	  continue;
	}
	else if (!strcmp (inbuf, "faceset")) {
	  face_info.want_faceset = strdup_local (cp);	/* memory leak ! */
	  continue;
	}
	/* legacy, as this is now just saved as 'lighting' */
	else if (!strcmp (inbuf, "per_tile_lighting")) {
	  if (val) want_config[CONFIG_LIGHTING] = CFG_LT_TILE;
	}
	else if (!strcmp (inbuf, "per_pixel_lighting")) {
	  if (val) want_config[CONFIG_LIGHTING] = CFG_LT_PIXEL;
	}
	else fprintf (stderr,"Unknown line in gdefaults: %s %s\n", inbuf, cp);
      }
      fclose (fp);
    }
  /* Make sure some of the values entered are sane - since a user can
   * edit the defaults file directly, they could put bogus values
   * in
   */
  if (want_config[CONFIG_ICONSCALE]< 25 || want_config[CONFIG_ICONSCALE]>200) {
    fprintf (stderr,"ignoring iconscale value read for gdefaults file.\n");
    fprintf (stderr,"Invalid iconscale range (%d), valid range for -iconscale is 25 through 200\n", want_config[CONFIG_ICONSCALE]);
    want_config[CONFIG_ICONSCALE] = use_config[CONFIG_ICONSCALE];
  }
  if (want_config[CONFIG_MAPSCALE]< 25 || want_config[CONFIG_MAPSCALE]>200) {
    fprintf (stderr,"ignoring mapscale value read for gdefaults file.\n");
    fprintf (stderr,"Invalid mapscale range (%d), valid range for -iconscale is 25 through 200\n", want_config[CONFIG_MAPSCALE]);
    want_config[CONFIG_MAPSCALE] = use_config[CONFIG_MAPSCALE];
  }
  if (!want_config[CONFIG_LIGHTING]) {
    fprintf (stderr,"No lighting mechanism selected - will not use darkness code\n");
    want_config[CONFIG_DARKNESS] = FALSE;
  }
    
  /* Make sure the map size os OK */
  if (want_config[CONFIG_MAPWIDTH] < 9 || want_config[CONFIG_MAPWIDTH] > MAP_MAX_SIZE) {
    fprintf (stderr,"Invalid map width (%d) option in gdefaults. Valid range is 9 to %d\n", want_config[CONFIG_MAPWIDTH], MAP_MAX_SIZE);
    want_config[CONFIG_MAPWIDTH] = use_config[CONFIG_MAPWIDTH];
  }
  if (want_config[CONFIG_MAPHEIGHT] < 9 || want_config[CONFIG_MAPHEIGHT] > MAP_MAX_SIZE) {
    fprintf (stderr,"Invalid map height (%d) option in gdefaults. Valid range is 9 to %d\n", want_config[CONFIG_MAPHEIGHT], MAP_MAX_SIZE);
    want_config[CONFIG_MAPHEIGHT] = use_config[CONFIG_MAPHEIGHT];
  }

  /* Now copy over the values just loaded */
  for (i=0; i<CONFIG_NUMS; i++) {
    use_config[i] = want_config[i];
  }
    
  allocate_map ( &the_map, FOG_MAP_SIZE, FOG_MAP_SIZE);
  pl_pos.x= the_map.x / 2;
  pl_pos.y= the_map.y / 2;
}

void save_defaults ()
{
  char path[MAX_BUF],buf[MAX_BUF];
  FILE *fp;
  int i;

  sprintf (path,"%s/.crossfire/gdefaults", getenv ("HOME"));
  if (make_path_to_file (path)==-1) {
    fprintf (stderr,"Could not create %s\n", path);
    return;
  }
  if ((fp=fopen (path,"w"))==NULL) {
    fprintf (stderr,"Could not open %s\n", path);
    return;
  }
  fprintf (fp,"# This file is generated automatically by gcfclient.\n");
  fprintf (fp,"# Manually editing is allowed, however gcfclient may be a bit finicky about\n");
  fprintf (fp,"# some of the matching it does.  all comparisons are case sensitive.\n");
  fprintf (fp,"# 'True' and 'False' are the proper cases for those two values\n");
  fprintf (fp,"# 'True' and 'False' have been replaced with 1 and 0 respectively\n");
  fprintf (fp,"server: %s\n", server);
  fprintf (fp,"faceset: %s\n", face_info.want_faceset);

  /* This isn't quite as good as before, as instead of saving things as 'True'
   * or 'False', it is just 1 or 0.  However, for the most part, the user isn't
   * going to be editing the file directly. 
   */
  for (i=1; i < CONFIG_NUMS; i++) {
    fprintf (fp,"%s: %d\n", config_names[i], want_config[i]);
  }

  fclose (fp);
  sprintf (buf,"Defaults saved to %s",path);
  draw_info (buf,NDI_BLUE);
}


/* init_windows:  This initiliazes all the windows - it is an
 * interface routine.  The command line arguments are passed to
 * this function to interpert.  Note that it is not in fact
 * required to parse anything, but doing at least -server and
 * -port would be a good idea.
 *
 * This function returns 0 on success, nonzero on failure.
 */

int init_windows (int argc, char **argv)
{
  pghandle main_pane, info_pane, inv_pane, map_image, vitals_box, input_filter, bar, it;
  int h, t_w, t_h;
  struct pgmemdata lf;

  /* pgui initialization */
  pgInit (argc,argv);
  if (argc > 1)
    server = argv[1];
  pgRegisterApp (PG_APP_NORMAL,"Crossfire Client",0);

  title_label = pgGetWidget (PGDEFAULT, PG_WP_PANELBAR_LABEL);

  info_pane = pgNewWidget (PG_WIDGET_PANEL, PG_DERIVE_INSIDE, PGDEFAULT);
  pgDelete (pgGetWidget (PGDEFAULT, PG_WP_PANELBAR_CLOSE));
  bar = pgGetWidget (info_pane, PG_WP_PANELBAR);
  h = pgGetWidget (bar, PG_WP_HEIGHT);
  is_textmode = h == 1;
  if (is_textmode)
    {
      map_widget_size = use_config[CONFIG_MAPHEIGHT];
      printf ("text mode client\n");
    }
  else
    {
      map_widget_size = use_config[CONFIG_MAPHEIGHT] * image_size;
      printf ("graphical client\n");
    }
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_RIGHT,
	       PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
	       PG_WP_SIZE, 50,
	       0);
  info_widget = pgNewWidget (PG_WIDGET_TERMINAL, PG_DERIVE_INSIDE, info_pane);
  pgSetWidget (info_widget,
	       PG_WP_SIDE,PG_S_ALL,
	       0);
  /* measure the desired width of the info terminal */
  if (!is_textmode)
    {
      it = pgNewString ("#################################################");
      pgSizeText (&t_w, &t_h,
		  pgGetWidget (info_widget, PG_WP_FONT), it);
      pgDelete (it);
      printf ("resizing info_pane to %d\n", t_w);
      pgSetWidget (info_pane,
		   PG_WP_SIZE, t_w + h,
		   0);
    }

  main_pane = pgNewWidget (PG_WIDGET_PANEL, PG_DERIVE_AFTER, info_pane);
  pgDelete (pgGetWidget (PGDEFAULT, PG_WP_PANELBAR_CLOSE));
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_RIGHT,
	       PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
	       PG_WP_SIZE, is_textmode ? 40 : (use_config[CONFIG_MAPHEIGHT] * image_size) + h,
	       0);
  stats_pane = pgNewWidget (PG_WIDGET_PANEL, PG_DERIVE_INSIDE, main_pane);
  pgDelete (pgGetWidget (PGDEFAULT, PG_WP_PANELBAR_CLOSE));
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_TOP,
	       PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
	       PG_WP_SIZE, is_textmode ? 5 : 100,
	       0);
  stats_tb = pgNewWidget (PG_WIDGET_TEXTBOX, PG_DERIVE_INSIDE, PGDEFAULT);
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("Stats come here, eventually"),
	       PG_WP_READONLY, 1,
	       0);

  map_widget = pgNewWidget (PG_WIDGET_LABEL, PG_DERIVE_AFTER, stats_pane);
  map_image = pgCreateBitmap(use_config[CONFIG_MAPWIDTH] * (is_textmode ? 2 : image_size),
			     use_config[CONFIG_MAPHEIGHT] * (is_textmode ? 1 : image_size));
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_TOP,
	       PG_WP_SIZEMODE, PG_SZMODE_PIXEL,
	       PG_WP_SIZE, map_widget_size,
	       PG_WP_IMAGE, map_image,
	       PG_WP_TRANSPARENT, 0,
	       0);

  vitals_box = pgNewWidget (PG_WIDGET_BOX, PG_DERIVE_AFTER, map_widget);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_TOP,
	       0);
  pgNewWidget (PG_WIDGET_LABEL, PG_DERIVE_INSIDE, PGDEFAULT);
  hp_indicator = pgNewWidget (PG_WIDGET_INDICATOR, PG_DERIVE_INSIDE, PGDEFAULT);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_SIZEMODE, PG_SZMODE_CNTFRACT,
	       PG_WP_SIZE, pgFraction (49, 100),
	       PG_WP_VALUE, 50,
	       0);
  hp_label = pgNewWidget (PG_WIDGET_LABEL, PG_DERIVE_INSIDE, PGDEFAULT);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_TEXT, pgNewString ("0/0"),
	       PG_WP_COLOR, 0,
	       0);
  food_indicator = pgNewWidget (PG_WIDGET_INDICATOR, PG_DERIVE_AFTER, hp_indicator);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_RIGHT,
	       PG_WP_SIZEMODE, PG_SZMODE_CNTFRACT,
	       PG_WP_SIZE, pgFraction (49, 100),
	       PG_WP_VALUE, 50,
	       0);
  food_label = pgNewWidget (PG_WIDGET_LABEL, PG_DERIVE_INSIDE, PGDEFAULT);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_TEXT, pgNewString ("0"),
	       PG_WP_COLOR, 0,
	       0);
  vitals_box = pgNewWidget (PG_WIDGET_BOX, PG_DERIVE_AFTER, vitals_box);
  sp_indicator = pgNewWidget (PG_WIDGET_INDICATOR, PG_DERIVE_INSIDE, PGDEFAULT);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_SIZEMODE, PG_SZMODE_CNTFRACT,
	       PG_WP_SIZE, pgFraction (49, 100),
	       PG_WP_VALUE, 50,
	       0);
  sp_label = pgNewWidget (PG_WIDGET_LABEL, PG_DERIVE_INSIDE, PGDEFAULT);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_TEXT, pgNewString ("0/0"),
	       PG_WP_COLOR, 0,
	       0);
  gr_indicator = pgNewWidget (PG_WIDGET_INDICATOR, PG_DERIVE_AFTER, sp_indicator);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_RIGHT,
	       PG_WP_SIZEMODE, PG_SZMODE_CNTFRACT,
	       PG_WP_SIZE, pgFraction (49, 100),
	       PG_WP_VALUE, 50,
	       0);
  gr_label = pgNewWidget (PG_WIDGET_LABEL, PG_DERIVE_INSIDE, PGDEFAULT);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_LEFT,
	       PG_WP_TEXT, pgNewString ("0/0"),
	       PG_WP_COLOR, 0,
	       0);
  resistances_tb = pgNewWidget (PG_WIDGET_TEXTBOX, PG_DERIVE_AFTER, vitals_box);
  pgWriteData (resistances_tb, pgFromMemory ("Resistances come here, eventually", 33));
  pgSetWidget (PGDEFAULT,
	       PG_WP_READONLY, 1,
	       PG_WP_INSERTMODE, PG_INSERT_APPEND,
	       0);


  inv_pane = pgNewWidget (PG_WIDGET_PANEL, PG_DERIVE_AFTER, main_pane);
  pgDelete (pgGetWidget (PGDEFAULT, PG_WP_PANELBAR_CLOSE));
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_TOP,
	       PG_WP_SIZEMODE, PG_SZMODE_PERCENT,
	       PG_WP_SIZE, 75,
	       0);
  weight_label = pgNewWidget (PG_WIDGET_LABEL, PG_DERIVE_INSIDE, PGDEFAULT);
  pgSetWidget (PGDEFAULT,
	       PG_WP_TEXT, pgNewString ("Carrying:"),
	       PG_WP_TRANSPARENT, 0,
	       0);
  temp_look_title = 0;
  look_label = pgGetWidget (inv_pane, PG_WP_PANELBAR_LABEL);
  default_look_title = pgNewString ("You see:");
  pgSetWidget (look_label,
	       PG_WP_TEXT, default_look_title,
	       0);
  look_items[0].item = pgNewWidget (PG_WIDGET_SCROLLBOX, PG_DERIVE_AFTER, inv_pane);
  inv_items[0].item = pgNewWidget (PG_WIDGET_SCROLLBOX, PG_DERIVE_AFTER, weight_label);
  looking_at = cpl.below;
  memset (inv_items + 1, 0, MAX_LIST_ITEMS * sizeof (struct list_item));
  memset (look_items + 1, 0, MAX_LIST_ITEMS * sizeof (struct list_item));

  pgUpdate (); /* so that the terminal widget realizes itself and we may clear it  */
  pgWriteData (info_widget, colorcmd[0]);
  pgWriteData (info_widget, pgFromMemory ("\033[20h\033[2J", 9));

  map_context = pgNewBitmapContext (map_image);
  pgSetColor (map_context, 0);
  pgRect (map_context, 0, 0,
	  use_config[CONFIG_MAPWIDTH] * (is_textmode ? 2 : image_size),
	  use_config[CONFIG_MAPHEIGHT] * (is_textmode ? 1 : image_size));

  input_filter = pgNewInFilter (pgGetServerRes (PGRES_INFILTER_KEY_PREPROCESS),
				PG_TRIGGER_CHAR | PG_TRIGGER_KEYDOWN, PG_TRIGGERS_KEY);

  pgBind (input_filter, PGBIND_ANY, &keyboard_handler, NULL);

  return 0;
}


/* A wrapper around PicoGUI's select () call so we can
 * check for activity on our pty too */
int mySelect (int n, fd_set *readfds, fd_set *writefds,
	      fd_set *exceptfds, struct timeval *timeout)
{
  /* Set up our fd */
  if (csocket.fd != -1)
    {
      if ((csocket.fd) >= n)
	n = csocket.fd + 1;
      FD_SET (csocket.fd, readfds);
    }

  /* Selectify things */
  return select (n,readfds,writefds,exceptfds,timeout);
}

/* Bottom-half for the select, allowed to make PicoGUI calls */
void mySelectBH (int result, fd_set *readfds)
{
  if (csocket.fd==-1)
    return;

  /* Is it for us? */
  if (result<=0 || !FD_ISSET (csocket.fd, readfds))
    return;

  DoClient (&csocket);
  draw_lists ();
  pgFlushRequests ();
}


int main (int argc, char **argv)
{
  /* crossfire initialization */
  init_client_vars ();
  memset (images, 0, MAXIMAGENUM * sizeof (struct image_data));
  // uncomment to allow metaserver negotiation
  //server = NULL;
  load_defaults (); 
  strcpy (VERSION_INFO, "PicoGUI Client " VERSION);
  want_skill_exp=1;
  use_config[CONFIG_SOUND] = FALSE;
  if (init_windows (argc, argv))
    {
      fprintf (stderr,"Failure to init windows.\n");
      exit (1);
    }
  csocket.inbuf.buf=malloc (MAXSOCKBUF);
  reset_client_vars ();
  csocket.inbuf.len=0;
  csocket.cs_version=0;

  if (server == NULL)
    {
      char *ms;
      server = SERVER;
      metaserver_get_info (meta_server, meta_port);
      metaserver_show (TRUE);
      do
	{
	  ms="1";
	  sleep (1);
	}
      while (metaserver_select (ms));
      negotiate_connection (use_config[CONFIG_SOUND]);
    }
  else
    {
      csocket.fd=init_connection (server, use_config[CONFIG_PORT]);
      if (csocket.fd == -1)
	{
	  fprintf (stderr, "Can't connect!\n");
	  exit (1);
	}
      negotiate_connection (use_config[CONFIG_SOUND]);
    }

  /* make libpgui use our select, which handles the server connection too
   */
  pgCustomizeSelect (&mySelect, &mySelectBH);

  /* make it so! */
  pgEventLoop ();
  return 0;
}
