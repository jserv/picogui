/* $Id$
 *
 * font_freetype.c - Font engine that uses Freetype2 to render
 *                   spiffy antialiased Type1 and TrueType fonts
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
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/font.h>
#include <pgserver/configfile.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_GLYPH_H
#include FT_IMAGE_H
#include FT_CACHE_CHARMAP_H
#include FT_CACHE_IMAGE_H

#define CFGSECTION "font-freetype"

void ft_build_gamma_table(u8 *table, float gamma);
u8 ft_light_gamma_table[256];
u8 ft_dark_gamma_table[256];
int ft_gamma_light_dark_threshold;     /* Premultiplied by 3 */
u8 *ft_pick_gamma_table(hwrcolor c);

struct ft_face_id {
  const char *file_path;
  const char *relative_path;  /* Path relative to our scanning path */
  struct font_style fs;
  struct ft_face_id *next;
};

struct freetype_fontdesc {
  struct ft_face_id *face;
  int size;
  int flags;
  struct font_metrics metrics;
};
#define DATA ((struct freetype_fontdesc *)self->data)

FT_Library         ft_lib;
struct ft_face_id* ft_facelist;
FT_Int32           ft_glyph_flags;
struct font_style  ft_default_fs;
struct font_style  ft_default_fixed_fs;
int                ft_minimum_size;
FTC_Manager        ft_cache_manager;
FTC_ImageCache     ft_image_cache;
FTC_CMapCache      ft_cmap_cache;

/* x,y pair in 26.6 fixed point */
struct pgpair26_6 {
  s32 x,y;
};

int ft_face_scan_list(const char *file, const char *path);
void ft_face_load(const char *file, int base_len);
int ft_fontcmp(const struct ft_face_id *f, const struct font_style *fs);
void ft_get_face_style(FT_Face f, struct font_style *fs);
void ft_get_descriptor_face(struct font_descriptor *self, FT_Face *aface);
static FT_Error ft_face_requester(FTC_FaceID face_id, FT_Library library,
				  FT_Pointer request_data, FT_Face *aface );
void ft_load_image(struct font_descriptor *self, int ch, FT_Glyph *g);
void ft_subpixel_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pgpair26_6 *position,
			   hwrcolor col, int ch, struct pgquad *clip, s16 lgop, s16 angle);
void ft_font_listing(void);
void ft_style_print(const struct font_style *fs);
void ft_style_scan(struct font_style *fs, const char *str);
char *ft_strdup(const char *str);

/************************************************* Initialization ***/

g_error freetype_engine_init(void) {
  g_error e;
  struct ft_face_id *f;
  const char *path;
  
  if (FT_Init_FreeType(&ft_lib))
    return mkerror(PG_ERRT_IO,119);   /* Error initializing font engine */

  /* Use Freetype's cache system */
  if (FTC_Manager_New(ft_lib,get_param_int(CFGSECTION,"max_faces",0),
		      get_param_int(CFGSECTION,"max_sizes",0),
		      get_param_int(CFGSECTION,"max_bytes",0),
		      &ft_face_requester,NULL,&ft_cache_manager))
    return mkerror(PG_ERRT_IO,119);   /* Error initializing font engine */

  /* Image cache for storing glyphs */
  if (FTC_ImageCache_New(ft_cache_manager,&ft_image_cache))
    return mkerror(PG_ERRT_IO,119);   /* Error initializing font engine */

  /* Character mapping cache */
  if (FTC_CMapCache_New(ft_cache_manager,&ft_cmap_cache))
    return mkerror(PG_ERRT_IO,119);   /* Error initializing font engine */

  /* Custom glyph flags */
  ft_glyph_flags = FT_LOAD_RENDER;
  if (get_param_int(CFGSECTION,"no_hinting",0))
    ft_glyph_flags |= FT_LOAD_NO_HINTING;
  if (get_param_int(CFGSECTION,"no_bitmap",1))
    ft_glyph_flags |= FT_LOAD_NO_BITMAP;
  if (get_param_int(CFGSECTION,"force_autohint",1))
    ft_glyph_flags |= FT_LOAD_FORCE_AUTOHINT;

  /* Default font and sizing config */
  ft_style_scan(&ft_default_fs,get_param_str(CFGSECTION,"default","Bitstream Vera Sans:10"));
  ft_style_scan(&ft_default_fixed_fs,get_param_str(CFGSECTION,"default_fixed","Bitstream Vera Sans Mono:10"));
  ft_minimum_size = get_param_int(CFGSECTION,"minimum_size",5);

  /* Gamma config/initialization */
  ft_build_gamma_table(ft_light_gamma_table,
		       atof(get_param_str(CFGSECTION,"light_gamma","1.5")));
  ft_build_gamma_table(ft_dark_gamma_table,
		       atof(get_param_str(CFGSECTION,"dark_gamma","0.75")));
  ft_gamma_light_dark_threshold = 3*get_param_int(CFGSECTION,"gamma_light_dark_threshold",128);

  /* Scan for available faces  */
  ft_facelist = NULL;
  path = get_param_str(CFGSECTION,"path","/usr/share/fonts");
  if (ft_face_scan_list(get_param_str(CFGSECTION,"scan_list",NULL),path))
    os_dir_scan(path, &ft_face_load);
  
  /* Dump scanned list if we're asked to */
  if (get_param_int(CFGSECTION,"dump_list",0)) {
    ft_font_listing();
    /* It's ok to exit here, fonts are initialized before drivers */
    exit(0);
  }

  if (!ft_facelist)
    return mkerror(PG_ERRT_IO,66);  /* Can't find fonts */

  return success;
}

void freetype_engine_shutdown(void) {
  struct ft_face_id *f = ft_facelist;
  struct ft_face_id *condemn;

  while (f) {
    condemn = f;
    f = f->next;
    g_free(condemn->file_path);
    g_free(condemn->fs.name);
    g_free(condemn);
  }

  if (ft_default_fs.name)
    g_free(ft_default_fs.name);
  if (ft_default_fixed_fs.name)
    g_free(ft_default_fixed_fs.name);

  FTC_Manager_Done(ft_cache_manager);
  FT_Done_FreeType(ft_lib);
}

/* Callback for loading font faces on demand */
static FT_Error ft_face_requester( FTC_FaceID   face_id,
				   FT_Library   library,
				   FT_Pointer   request_data,
				   FT_Face     *aface ) {
  struct ft_face_id *f = (struct ft_face_id *) face_id;  
  return FT_New_Face( library, f->file_path, 0, aface );
}

/* Store a ft_face_id for the specified font if it loads 
 */
void ft_face_load(const char *file, int base_len) {
  FT_Face face;
  struct ft_face_id *pfid;

  /* We must allocate the new node now (and free it if this fails)
   * since the face cache uses the _pointer_ as the cache key, not
   * the contents!
   */
  if (iserror(g_malloc((void**) &pfid, sizeof(struct ft_face_id))))
    return;

  pfid->file_path = file;
  if (FTC_Manager_Lookup_Face(ft_cache_manager,(FTC_FaceID) pfid, &face)) {
    g_free(pfid);
    return;
  }

  /* Set up the face's style, making sure to dynamically allocate
   * the face name and path!
   */
  ft_get_face_style(face, &pfid->fs);
  pfid->file_path = ft_strdup(file);
  pfid->fs.name = ft_strdup(pfid->fs.name);
  pfid->relative_path = pfid->file_path + base_len + 1;

  pfid->next = ft_facelist;
  ft_facelist = pfid;
}

/* Try to scan a file full of font face information, return nonzero on failure.
 * The fonts' paths are assumed to be relative to 'path'
 */
int ft_face_scan_list(const char *file, const char *path) {
  FILE *f;
  char buf[256];
  char *filename;
  char *style;
  struct ft_face_id *pfid;

  if (!file || !(f = fopen(file,"r")))
    return 1;

  while (fgets(buf, sizeof(buf)-1, f)) {
    /* Chop off newline */
    if (!*buf)
      continue;
    buf[strlen(buf)-1] = 0;

    /* Split the line into file and style */
    filename = buf;
    style = strchr(buf,' ');
    if (!style)
      continue;
    while (*style) {
      if (isspace(*style))
	*style = 0;
      else
	break;
      style++;
    }
    
    /* Allocate our new face ID node */
    if (iserror(g_malloc((void**) &pfid, sizeof(struct ft_face_id))))
      return 1;

    /* Allocate a buffer for the path */
    if (iserror(g_malloc((void**) &pfid->file_path, strlen(filename)+strlen(path)+2)))
      return 1;
    strcpy((char*) pfid->file_path, path);
    strcat((char*) pfid->file_path, "/");
    strcat((char*) pfid->file_path, filename);
    pfid->relative_path = pfid->file_path + strlen(path) + 1;

    /* Process the style string */
    ft_style_scan(&pfid->fs, style);

    pfid->next = ft_facelist;
    ft_facelist = pfid;
  }

  fclose(f);
}


/************************************************* Rendering ***/

/* Wrapper for ft_subpixel_draw_char, rounding to whole pixels */
void freetype_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pgpair *position,
		   hwrcolor col, int ch, struct pgquad *clip, s16 lgop, s16 angle) {
  struct pgpair26_6 subpos;
  subpos.x = ((s32)position->x) << 6;
  subpos.y = ((s32)position->y) << 6;
  ft_subpixel_draw_char(self,dest,&subpos,col,ch,clip,lgop,angle);
  position->x = subpos.x >> 6;
  position->y = subpos.y >> 6;
}

/* Guts of freetype_draw_char, using subpixel units */
void ft_subpixel_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pgpair26_6 *position,
			   hwrcolor col, int ch, struct pgquad *clip, s16 lgop, s16 angle) {
  int x,y,i,j;
  FT_Glyph g;
  FT_BitmapGlyph bg;

  x = position->x >> 6;
  y = position->y >> 6;

  /* If the character is completely outside the clipping rectangle, stop now.
   * Note that to determine this accurately we'd need to load the glyph, and we'd
   * like to avoid that if possible. This just uses the maximum character cell size.
   */
  switch (angle) {
  case 0:
    if (x > clip->x2 ||
	y > clip->y2 ||
	y + DATA->metrics.charcell.h < clip->y1)
      return;
    if (x + DATA->metrics.charcell.w < clip->x1)
      lgop = PG_LGOP_NULL;
    break;
  case 90:
    if (x > clip->x2 ||
	y < clip->y1 ||
	x + DATA->metrics.charcell.h < clip->x1)
      return;
    if (y - DATA->metrics.charcell.w > clip->y2)
      lgop = PG_LGOP_NULL;
    break;
  case 180:
    if (x < clip->x1 ||
	y < clip->y1 ||
	y - DATA->metrics.charcell.h > clip->y2)
      return;
    if (x - DATA->metrics.charcell.w > clip->x2)
      lgop = PG_LGOP_NULL;
    break;
  case 270:
    if (x < clip->x1 ||
	y > clip->y2 ||
	x - DATA->metrics.charcell.h > clip->x2)
      return;
    if (y + DATA->metrics.charcell.w < clip->y1)
      lgop = PG_LGOP_NULL;
    break;
  }

  /* Fetch a glyph from the cache, or render a new one */
  ft_load_image(self,ch,&g);
  bg = (FT_BitmapGlyph) g;

  if (lgop != PG_LGOP_NULL) {
    
    /* PicoGUI's character origin is at the top-left of the bounding box.
     * Add the ascent to reach the baseline, then subtract the bitmap origin
     * from that.
     */
    i = bg->left;
    j = DATA->metrics.ascent - bg->top;
    switch (angle) {
    case 0:
      x += i;
      y += j;
      break;
    case 90:
      x += j;
      y -= i;
      break;
    case 180:
      x -= i;
      y -= j;
      break;
    case 270:
      x -= j;
      y += i;
      break;
    }
    
    switch (bg->bitmap.pixel_mode) {
      
    case ft_pixel_mode_grays:
      VID(alpha_charblit)(dest,bg->bitmap.buffer,x,y,bg->bitmap.width,
			  bg->bitmap.rows,bg->bitmap.pitch,ft_pick_gamma_table(col),
			  angle,col,clip,lgop);
      break;
      
    case ft_pixel_mode_mono:
      VID(charblit) (dest,bg->bitmap.buffer,x,y,bg->bitmap.width,bg->bitmap.rows,
		     0,angle,col,clip,lgop, bg->bitmap.pitch);
      break;
      
    }
  }    

  switch (angle) {
  case 0:
    position->x += g->advance.x >> 10;
    position->y += g->advance.y >> 10;
    break;
  case 90:
    position->x += g->advance.y >> 10;
    position->y -= g->advance.x >> 10;
    break;
  case 180:
    position->x -= g->advance.x >> 10;
    position->y -= g->advance.y >> 10;
    break;
  case 270:
    position->x -= g->advance.y >> 10;
    position->y += g->advance.x >> 10;
    break;
  }
}

/* Our own version of draw_string that handles subpixel pen movement 
 */
void freetype_draw_string(struct font_descriptor *self, hwrbitmap dest, struct pgpair *position,
			  hwrcolor col, const struct pgstring *str, struct pgquad *clip,
			  s16 lgop, s16 angle) {
  struct pgstr_iterator p;
  int margin,lh,b,ch;
  struct pgpair26_6 subpos;

  pgstring_seek(str,&p,0,PGSEEK_SET);
  subpos.x = ((s32)position->x) << 6;
  subpos.y = ((s32)position->y) << 6;

  margin = DATA->metrics.margin << 6;
  lh = DATA->metrics.lineheight << 6;

  switch (angle) {
    
  case 0:
    subpos.x += margin;
    subpos.y += margin;
    b = subpos.x;
    break;
    
  case 90:
    subpos.x += margin;
    subpos.y -= margin;
    b = subpos.y;
    break;
    
  case 180:
    subpos.x -= margin;
    subpos.y -= margin;
    b = subpos.x;
    break;

  case 270:
    subpos.x -= margin;
    subpos.y += margin;
    b = subpos.y;
    break;
  }
  
  while ((ch = pgstring_decode(str,&p))) {
    if (ch=='\n')
      switch (angle) {
	
      case 0:
	subpos.y += lh;
	subpos.x = b;
	break;
	
      case 90:
	subpos.x += lh;
	subpos.y = b;
	break;
	
      case 180:
	subpos.y -= lh;
	subpos.x = b;
	break;
	
      case 270:
	subpos.x -= lh;
	subpos.y = b;
	break;
	
      }
    else if (ch!='\r') {
      ft_subpixel_draw_char(self,dest,&subpos,col,ch,clip,lgop,angle);
    }
  }

  position->x = subpos.x >> 6;
  position->y = subpos.y >> 6;
}

void ft_build_gamma_table(u8 *table, float gamma) {
  int i;
  for (i=0;i<256;i++)
    table[i] = 255 * pow(i/255.0, 1/gamma);
}

/* Use one table for light fonts, another for dark fonts */
u8 *ft_pick_gamma_table(hwrcolor c) {
  pgcolor pgc = vid->color_hwrtopg(c);
  if (getred(pgc)+getgreen(pgc)+getblue(pgc) >= ft_gamma_light_dark_threshold)
    return ft_light_gamma_table;
  return ft_dark_gamma_table;
}

/************************************************* Font Descriptors ***/

g_error freetype_create(struct font_descriptor *self, const struct font_style *fs) {
  g_error e;
  int size;
  struct font_style defaultfs;
  int r, closeness = -1;
  struct ft_face_id *closest, *f;
  FT_Face face;

  e = g_malloc((void**)&self->data,sizeof(struct freetype_fontdesc));
  errorcheck;

  if (!fs) {
    fs = &defaultfs;
    defaultfs.name = NULL;
    defaultfs.size = 0;
    defaultfs.style = PG_FSTYLE_DEFAULT;
    defaultfs.representation = 0;
  }

  /* If they asked for a default font, give it to them */
  if (!fs->name || !*fs->name || (fs->style & PG_FSTYLE_DEFAULT)) {
    int saved_size = fs->size;
    int saved_style = fs->style;

    memcpy(&defaultfs,
	   fs->style & PG_FSTYLE_FIXED ? &ft_default_fixed_fs : &ft_default_fs,
	   sizeof(defaultfs));
    defaultfs.size = saved_size;
    defaultfs.style |= saved_style;
    fs = &defaultfs;
  }

  /* Pick the closest font face */
  for (f=ft_facelist;f;f=f->next) {
    r = ft_fontcmp(f,fs);
    if (r > closeness) {
      closeness = r;
      closest = f;
    }
  }
  DATA->face = closest;

  /* If the font is scalable, pick the size intelligently, otherwise
   * we have to use whatever was chosen above.
   */
  if (closest->fs.representation & PG_FR_SCALABLE) {
    DATA->size = fs->size ? fs->size : 
      (fs->style & PG_FSTYLE_FIXED ? &ft_default_fixed_fs : &ft_default_fs)->size;
    if (DATA->size < ft_minimum_size)
      DATA->size = ft_minimum_size;
  }
  else
    DATA->size = closest->fs.size;
  DATA->flags = fs->style;

  /* Store the font metrics now, since they'll be needed frequently */
  ft_get_descriptor_face(self,&face);

  DATA->metrics.ascent = face->size->metrics.ascender >> 6;
  DATA->metrics.descent = (-face->size->metrics.descender) >> 6;
  DATA->metrics.lineheight = face->size->metrics.height >> 6;
  DATA->metrics.linegap = DATA->metrics.lineheight - 
    DATA->metrics.ascent - DATA->metrics.descent;
  DATA->metrics.charcell.w = face->size->metrics.max_advance >> 6;
  DATA->metrics.charcell.h = DATA->metrics.ascent + DATA->metrics.descent;

  if (DATA->flags & PG_FSTYLE_FLUSH)
    DATA->metrics.margin = 0;
  else
    DATA->metrics.margin = DATA->metrics.descent;

  return success;
}

void freetype_destroy(struct font_descriptor *self) {
  g_free(DATA);
}

/* Gauge the closeness between a face and a requested style
 */
int ft_fontcmp(const struct ft_face_id *f, const struct font_style *fs) {
  int result;
  int szdif;
  struct font_style f_fs;

  if (fs->size) { 
    if (f->fs.representation & PG_FR_SCALABLE)
      szdif = 0;
    else
      szdif = f->fs.size - fs->size;

    if (szdif<0) szdif = 0-szdif;
    result = FCMP_SIZE(szdif);
  }    
  else {
    result = 0;
  }

  if (fs->name && (!strcasecmp(f->fs.name,fs->name)))
    result |= FCMP_NAME;
  if ((f->fs.style&PG_FSTYLE_FIXED)==(fs->style&PG_FSTYLE_FIXED))
    result |= FCMP_FIXEDVAR;
  if ((f->fs.style&PG_FSTYLE_STYLE_MASK)==(fs->style&PG_FSTYLE_STYLE_MASK)) 
    result |= FCMP_STYLE;
  if ((f->fs.style&PG_FSTYLE_TYPE_MASK)==(fs->style&PG_FSTYLE_TYPE_MASK)) 
    result |= FCMP_TYPE;

  return result;
}

void ft_get_descriptor_face(struct font_descriptor *self, FT_Face *aface) {
  FTC_FontRec fr;

  fr.face_id = DATA->face;
  fr.pix_width = 0;
  fr.pix_height = DATA->size;
  FTC_Manager_Lookup_Size(ft_cache_manager,&fr,aface,NULL);
}

void ft_load_image(struct font_descriptor *self, int ch, FT_Glyph *g) {
  FTC_CMapDescRec cmapd;
  FTC_ImageTypeRec imgd;

  cmapd.face_id = (FTC_FaceID) DATA->face;
  cmapd.type = FTC_CMAP_BY_INDEX;
  cmapd.u.index = 0;

  imgd.font.face_id = DATA->face;
  imgd.font.pix_width = 0;
  imgd.font.pix_height = DATA->size;
  imgd.flags = ft_glyph_flags;

  FTC_ImageCache_Lookup(ft_image_cache, &imgd,
			FTC_CMapCache_Lookup(ft_cmap_cache, &cmapd, ch),
			g, NULL);
}

/************************************************* Metrics ***/

void freetype_measure_char(struct font_descriptor *self, struct pgpair *position,
		      int ch, s16 angle) {
  FT_Glyph g;
  ft_load_image(self,ch,&g);
  switch (angle) {
  case 0:
    position->x += g->advance.x >> 16;
    position->y += g->advance.y >> 16;
    break;
  case 90:
    position->x += g->advance.y >> 16;
    position->y -= g->advance.x >> 16;
    break;
  case 180:
    position->x -= g->advance.x >> 16;
    position->y -= g->advance.y >> 16;
    break;
  case 270:
    position->x -= g->advance.y >> 16;
    position->y += g->advance.x >> 16;
    break;
  }
}

void freetype_getmetrics(struct font_descriptor *self, struct font_metrics *m) {
  memcpy(m,&DATA->metrics,sizeof(struct font_metrics));
}

/* Our own version of measure_string that handles subpixel pen movement 
 */
void freetype_measure_string(struct font_descriptor *self, const struct pgstring *str,
			     s16 angle, s16 *w, s16 *h) {
  int max_x = 0, ch, x, y, original_x;
  struct pgstr_iterator p;
  FT_Glyph g;

  pgstring_seek(str,&p,0,PGSEEK_SET);
  original_x = x = DATA->metrics.margin << 7;

  /* Centering looks best when we shave 1/2 the descent off
   * the line height when computing string height.
   */
  y = x + ((DATA->metrics.lineheight - DATA->metrics.linegap - 
	    (DATA->metrics.descent >> 1)) << 6);

  while ((ch = pgstring_decode(str,&p))) {
    if (ch=='\n') {
      y += DATA->metrics.lineheight << 6;
      if (x>max_x) max_x = x;
      x = original_x;
    }
    else if (ch!='\r') {
      ft_load_image(self,ch,&g);
      x += g->advance.x >> 10;
      y += g->advance.y >> 10;
    }
  }
  if (x>max_x) max_x = x;

  if (angle==90 || angle==270) {
    *h = max_x >> 6;
    *w = y >> 6;
  }
  else {
    *w = max_x >> 6;
    *h = y >> 6;
  }
}


/************************************************* Enumeration ***/

void freetype_getstyle(int i, struct font_style *fs) {
  struct ft_face_id *f;

  /* Iterate to the face they asked about */
  for (f=ft_facelist;f && i>0;i--,f=f->next);
  if (f)
    memcpy(fs,&f->fs,sizeof(*fs));
  else
    memset(fs,0,sizeof(*fs));
}

void ft_get_face_style(FT_Face f, struct font_style *fs) {
  int i;
  memset(fs,0,sizeof(*fs));

  fs->name = f->family_name;

  if (f->style_flags & FT_STYLE_FLAG_ITALIC)    fs->style |= PG_FSTYLE_ITALIC;
  if (f->style_flags & FT_STYLE_FLAG_BOLD)      fs->style |= PG_FSTYLE_BOLD;
  if (f->face_flags & FT_FACE_FLAG_SCALABLE)    fs->representation |= PG_FR_SCALABLE;
  if (f->face_flags & FT_FACE_FLAG_FIXED_WIDTH) fs->style |= PG_FSTYLE_FIXED;

  for (i=0;i<f->num_charmaps;i++) {
    switch (f->charmaps[i]->encoding) {

    case ft_encoding_unicode:
      fs->style |= PG_FSTYLE_ENCODING_UNICODE;
      break;

    case ft_encoding_latin_1:
      fs->style |= PG_FSTYLE_ENCODING_ISOLATIN1;
      break;

    case ft_encoding_symbol:
      fs->style |= PG_FSTYLE_SYMBOL;
      break;
    }
  }

  if (f->available_sizes > 0)
    fs->size = f->available_sizes->height;

  /* Is there no better way to detect a condensed font? */
  if (f->style_name && strstr(f->style_name,"ondensed"))
    fs->style |= PG_FSTYLE_CONDENSED;

  /* If the font has "Mono" in the name, trust that it's a fixed width
   * (Added because Nimbus Mono L doesn't have the fixedwidth flag set)
   */
  if (f->family_name && strstr(f->family_name,"Mono"))
    fs->style |= PG_FSTYLE_FIXED;

}


/************************************************* Human-readable styles ***/

/* This uses the same format as cli_python, i.e.
 *  name:size:flags
 */

struct ft_flag {
  const char *name;
  int value;
};

struct ft_flag ft_styleflags[] = {
  {"fixed",              PG_FSTYLE_FIXED},
  {"default",            PG_FSTYLE_DEFAULT},
  {"symbol",             PG_FSTYLE_SYMBOL},
  {"subset",             PG_FSTYLE_SUBSET},
  {"encoding isolatin1", PG_FSTYLE_ENCODING_ISOLATIN1},
  {"encoding ibm",       PG_FSTYLE_ENCODING_IBM},
  {"doublespace",        PG_FSTYLE_DOUBLESPACE},
  {"bold",               PG_FSTYLE_BOLD},
  {"italic",             PG_FSTYLE_ITALIC},
  {"underline",          PG_FSTYLE_UNDERLINE},
  {"strikeout",          PG_FSTYLE_STRIKEOUT},
  {"grayline",           PG_FSTYLE_GRAYLINE},
  {"flush",              PG_FSTYLE_FLUSH},
  {"doublewidth",        PG_FSTYLE_DOUBLEWIDTH},
  {"italic2",            PG_FSTYLE_ITALIC2},
  {"encoding unicode",   PG_FSTYLE_ENCODING_UNICODE},
  {"condensed",          PG_FSTYLE_CONDENSED},
  {NULL,0}
};

struct ft_flag ft_repflags[] = {
  {"bitmap normal",      PG_FR_BITMAP_NORMAL},
  {"bitmap bold",        PG_FR_BITMAP_BOLD},
  {"bitmap italic",      PG_FR_BITMAP_ITALIC},
  {"bitmap bolditalic",  PG_FR_BITMAP_BOLDITALIC},
  {"scalable",           PG_FR_SCALABLE},
  {NULL,0}
};

void ft_font_listing(void) {
  struct ft_face_id *f;

  for (f=ft_facelist;f;f=f->next) {
    fprintf(stderr, "%-50s",f->relative_path);
    ft_style_print(&f->fs);
    fprintf(stderr, "\n");
  }
}

void ft_style_print(const struct font_style *fs) {
  struct ft_flag *flag;
  
  fprintf(stderr, "%s:%d",fs->name,fs->size);

  for (flag=ft_styleflags;flag->name;flag++)
    if (flag->value & fs->style)
      fprintf(stderr, ":%s",flag->name);

  for (flag=ft_repflags;flag->name;flag++)
    if (flag->value & fs->representation)
      fprintf(stderr, ":%s",flag->name);
}

/* Scan a human-readable style string back into a
 * struct font_style, dynamically allocating the name.
 */
void ft_style_scan(struct font_style *fs, const char *str) {
  const char *p;
  int token_len;
  struct ft_flag *flag;
  memset(fs,0,sizeof(*fs));

  if (!str || !*str)
    return;

  /* The text before the first colon is the name */
  p = strchr(str,':');
  if (!p) {
    /* No colon? Assume it's a bare name */
    fs->name = ft_strdup(str);
    return;
  }
  if (p!=str) {
    if (iserror(g_malloc((void**) &fs->name, p-str+1)))
      return;
    ((char*)fs->name)[p-str] = 0;
    strncpy((char*) fs->name, str, p-str);
  }
  str = p+1;

  /* The text between this colon and the next is the size */
  fs->size = atoi(str);
  p = strchr(str,':');
  if (!p)
    return;
  str = p+1;

  /* Now the rest of the string can be any of the flags */
  for (;;) {
    p = strchr(str,':');
    if (p)
      token_len = p - str;
    else
      token_len = strlen(str);

    /* Now scan for the flag value */
    for (flag=ft_styleflags;flag->name;flag++)
      if (!strncasecmp(flag->name,str,token_len) && !flag->name[token_len])
	fs->style |= flag->value;
    for (flag=ft_repflags;flag->name;flag++)
      if (!strncasecmp(flag->name,str,token_len) && !flag->name[token_len])
	fs->representation |= flag->value;

    if (p) 
      str = p+1;
    else
      break;
  }
}

/* g_malloc and strcpy wrapper, returns NULL on failure */
char *ft_strdup(const char *str) {
  char *s;

  if (iserror(g_malloc((void**) &s, strlen(str)+1)))
    return NULL;
  strcpy(s,str);
  return s;
}

/************************************************* Registration ***/

g_error freetype_regfunc(struct fontlib *f) {
  f->engine_init = &freetype_engine_init;
  f->engine_shutdown = &freetype_engine_shutdown;
  f->draw_char = &freetype_draw_char;
  f->measure_char = &freetype_measure_char;
  f->create = &freetype_create;
  f->destroy = &freetype_destroy;
  f->getstyle = &freetype_getstyle;
  f->getmetrics = &freetype_getmetrics;
  f->measure_string = &freetype_measure_string;
  f->draw_string = &freetype_draw_string;
  return success;
}

/* The End */

