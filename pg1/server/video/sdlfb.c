/* $Id$
 *
 * sdlfb.c - This driver provides an interface between the linear VBLs
 *           and a framebuffer provided by the SDL graphics library.
 *           It can run on many output devices, but SDL is most often used
 *           with the X window system. This SDL driver has facilities for
 *           simulating embedded platforms: low-bpp emulation and Skins.
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

#include <pgserver/common.h>      /* Needed for any pgserver file */

#include <pgserver/video.h>       /* Always needed for a video driver! */
#include <pgserver/render.h>      /* For data types like 'quad' */
#include <pgserver/input.h>       /* For loading our corresponding input lib */
#include <pgserver/configfile.h>  /* For loading our configuration */

#include <SDL/SDL.h>              /* This is the SDL video driver */

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#ifdef CONFIG_SDLSKIN
#include <stdio.h>                /* File I/O for loading skin bitmap */
#include <stdlib.h>               /* strtol() */
#elif defined(CONFIG_SDLSDC)
#include <stdlib.h>               /* strtol() */
#endif
#ifndef WIN32
#include <unistd.h>               /* write() for beeping kludge */
#else
#include <io.h>               /* write() for beeping kludge */
#endif
#include <string.h>

SDL_Surface *sdl_vidsurf;
#if defined(CONFIG_SDLEMU_COLOR) || defined(CONFIG_SDLEMU_BLIT)
int sdlfb_emucolors;
#endif

/* config data for skins */
#ifdef CONFIG_SDLSKIN
pgcolor sdlfb_tint;
s16 sdlfb_display_x;
s16 sdlfb_display_y;
u16 sdlfb_simbits;
u16 sdlfb_scale;
#endif

#ifdef CONFIG_SDLSDC
/* config data for secondary display channel */
pgcolor sdlsdc_fg,sdlsdc_bg;
handle sdlsdc_font;
s16 sdlsdc_x,sdlsdc_y;
s16 sdlsdc_w,sdlsdc_h;
 /* variables */
hwrcolor sdlsdc_hfg,sdlsdc_hbg;
struct pgpair sdlsdc_c;
struct stdbitmap sdlsdc_bits;

#endif

#if defined(CONFIG_SDLEMU_BLIT) || defined(CONFIG_SDLSKIN)
u8 *sdlfb_backbuffer;
#endif

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

hwrcolor sdlfbemu_color_pgtohwr(pgcolor c);
pgcolor sdlfbemu_color_hwrtopg(hwrcolor c);
g_error sdlfb_init(void);
void sdlfb_close(void);
void sdlfb_update(hwrbitmap d,s16 x,s16 y,s16 w,s16 h);
g_error sdlfb_regfunc(struct vidlib *v);
g_error sdlfb_setmode(s16 xres,s16 yres,s16 bpp,u32 flags);
hwrcolor sdlfb_tint_pgtohwr(pgcolor c);
pgcolor sdlfb_color_tint(pgcolor c);
hwrcolor sdlfb_tint_hwrtopg(pgcolor c);
pgcolor sdlfb_color_untint(pgcolor c);
g_error sdlfb_sdc_char(char c);

g_error sdlfb_init(void) {
  /* Avoid freeing a nonexistant backbuffer in close() */
  FB_MEM = NULL;
   
  /* Default mode: 640x480 */
  if (!vid->xres) vid->xres = 640;
  if (!vid->yres) vid->yres = 480;

  /* Start up the SDL video subsystem thingy */
  if (SDL_Init(SDL_INIT_VIDEO))
    return mkerror(PG_ERRT_IO,46);

  /* Load a main input driver */
  return load_inlib(&sdlinput_regfunc,&inlib_main);
}

g_error sdlfb_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  u32 sdlflags = SDL_RESIZABLE;
  char str[80];
  SDL_Color palette[256];
  int i;
  s16 fbw,fbh;
#ifdef CONFIG_SDLSKIN
  const char *s;
#endif

#ifdef CONFIG_SDLEMU_BLIT
   /* Make screen divisible by a byte */
  if (bpp && bpp<8)
     xres &= ~((8/bpp)-1);
#endif

#ifdef CONFIG_SDLSKIN
  sdlfb_scale = get_param_int("video-sdlfb","scale",1);
  fbw = xres * sdlfb_scale;
  fbh = yres * sdlfb_scale;
#else
  fbw = xres;
  fbh = yres;
#endif  


  /* Interpret flags */
  if (get_param_int("video-sdlfb","fullscreen",0))
    sdlflags |= SDL_FULLSCREEN;
#ifdef CONFIG_SDLSKIN
  if ((i = get_param_int("video-sdlfb","width",0))) {
    fbw = i;
    sdlflags &= ~SDL_RESIZABLE;
  }
  if ((i = get_param_int("video-sdlfb","height",0))) {
    fbh = i;
    sdlflags &= ~SDL_RESIZABLE;
  }
  sdlfb_simbits = get_param_int("video-sdlfb","simbits",0);
  sdlfb_tint = strtol(get_param_str("video-sdlfb","tint","FFFFFF"),NULL,16);
#endif
#ifdef CONFIG_SDLSDC
  sdlsdc_fg = strtol(get_param_str("video-sdlfb","sdc_fg","000000"),NULL,16);
  sdlsdc_bg = strtol(get_param_str("video-sdlfb","sdc_bg","FFFFFF"),NULL,16);
  sdlsdc_x  = get_param_int("video-sdlfb","sdc_x",0);
  sdlsdc_y  = get_param_int("video-sdlfb","sdc_y",0);
  sdlsdc_w  = get_param_int("video-sdlfb","sdc_w",0);
  sdlsdc_h  = get_param_int("video-sdlfb","sdc_h",0);
  /* We can't load the font yet because functions findfont() rely on
   * haven't been initialized yet. Save that for later.
   */
#endif

#ifdef CONFIG_SDLEMU_BLIT
   /* Make screen divisible by a byte */
  if (bpp && bpp<8)
     fbw &= ~((8/bpp)-1);
#endif

#ifdef CONFIG_SDLEMU_BLIT
  /* Free the backbuffer */
  if (vid->bpp && (vid->bpp<8) && FB_MEM) {
     g_free(FB_MEM);
     FB_MEM = NULL;
  }
#endif
   
  /* Set the video mode */
  if ((!sdl_vidsurf) || xres != vid->xres || 
      yres !=vid->yres || bpp != vid->bpp) {
     sdl_vidsurf = SDL_SetVideoMode(fbw,fbh,(bpp && bpp<8) ? 8 : bpp,sdlflags);
     if (!sdl_vidsurf)
       return mkerror(PG_ERRT_IO,47);
  }
     
  /* Use the default depth? */
  if (!bpp)
     bpp  = sdl_vidsurf->format->BitsPerPixel;

  /* If we're emulating low bpp with color conversion, load custom
   * color functions and a palette */
#if defined(CONFIG_SDLEMU_COLOR) || defined(CONFIG_SDLEMU_BLIT)
  if (bpp<8) {
     int colors = 1<<bpp;
     pgcolor pc;
     sdlfb_emucolors = colors-1;
     for (i=0;i<colors;i++) {
	pc = sdlfbemu_color_hwrtopg(i);
	palette[i].r = getred(pc);
	palette[i].g = getgreen(pc);
	palette[i].b = getblue(pc);
     }
     SDL_SetColors(sdl_vidsurf,palette,0,colors);
#ifdef CONFIG_SDLEMU_COLOR
     bpp = 8;
#endif
  }
   else
     sdlfb_emucolors = 0;
#endif /* defined(CONFIG_SDLEMU_COLOR) || defined(CONFIG_SDLEMU_BLIT) */
   
  /* Load a VBL */
  switch (bpp) {
     
#ifdef CONFIG_SDLEMU_BLIT
     /* Low bit depths */
     
#ifdef CONFIG_VBL_LINEAR1
   case 1:
     setvbl_linear1(vid);
     break;
#endif

#ifdef CONFIG_VBL_LINEAR2
   case 2:
     setvbl_linear2(vid);
     break;
#endif

#ifdef CONFIG_VBL_LINEAR4
   case 4:
     setvbl_linear4(vid);
     break;
#endif

#endif /* CONFIG_SDLEMU_BLIT */
     
#ifdef CONFIG_VBL_LINEAR8
   case 8:
     setvbl_linear8(vid);

#ifdef CONFIG_SDLEMU_COLOR
     if (sdlfb_emucolors) {
	vid->color_pgtohwr = &sdlfbemu_color_pgtohwr;
	vid->color_hwrtopg = &sdlfbemu_color_hwrtopg;
     }
     else
       /* If this is 8bpp set up palette defined by the VBL */
#endif
     {
       pgcolor pgc;
       vid->bpp = bpp;
       for (i=0;i<256;i++) {
	 pgc = vid->color_hwrtopg(i);
	 palette[i].r = getred(pgc);
	 palette[i].g = getgreen(pgc);
	 palette[i].b = getblue(pgc);
       }
       SDL_SetColors(sdl_vidsurf,palette,0,256);
     }
     break;
#endif
     
#ifdef CONFIG_VBL_LINEAR16
   case 16:
     setvbl_linear16(vid);
     break;
#endif

#ifdef CONFIG_VBL_LINEAR24
   case 24:
     setvbl_linear24(vid);
     break;
#endif

#ifdef CONFIG_VBL_LINEAR32
   case 32:
     setvbl_linear32(vid);
     break;
#endif

   default:
      sdlfb_close();
      return mkerror(PG_ERRT_BADPARAM,101);   /* Unknown bpp */
  }

#ifdef CONFIG_VBL_SLOWVBL
  if (get_param_int("video-sdlfb","slowvbl",0))
    setvbl_slowvbl(vid);
#endif
   
  /* Save the new video mode. Might not be the actual SDL video mode if we
   * are using a skin.
   */
  vid->xres = xres;
  vid->yres = yres;
   
#ifdef CONFIG_SDLEMU_BLIT
  /* If we're blitting to a higher bpp, use another backbuffer */
  if (bpp<8) {
     g_error e;
     
     vid->bpp = bpp;
     FB_BPL = (fbw * bpp) >> 3;
     /* The +1 allows blits some margin to read unnecessary bytes.
      * Keeps linear1's blit from triggering electric fence when the
      * cursor is put in the bottom-right corner ;-)
      */
     e = g_malloc((void**)&FB_MEM,(FB_BPL * fbh) + 1);
     errorcheck;
     sdlfb_backbuffer = FB_MEM;
  }
  else
#endif
  {
     ((struct stdbitmap*)vid->display)->bpp = vid->bpp  = sdl_vidsurf->format->BitsPerPixel;
     FB_MEM = sdl_vidsurf->pixels;
     FB_BPL = sdl_vidsurf->pitch;
  }
   
  /* Info */
  snprintf(str,sizeof(str),get_param_str("video-sdlfb","caption","PicoGUI (sdlfb@%dx%dx%d)"),
	   vid->xres,vid->yres,bpp);
  SDL_WM_SetCaption(str,NULL);

#ifdef CONFIG_SDLSDC
  /* Fabricate a bitmap specifying the sdc area */
  memcpy(&sdlsdc_bits,vid->display,sizeof(struct stdbitmap));
  sdlsdc_bits.freebits = 0;
  sdlsdc_bits.bits += bpp * sdlsdc_x / 8 + FB_BPL * sdlsdc_y;
  sdlsdc_bits.w = sdlsdc_w;
  sdlsdc_bits.h = sdlsdc_h;
#endif

#ifdef CONFIG_SDLSKIN
  /* Install the skin background */
  if ((s = get_param_str("video-sdlfb","background",NULL))) {
    FILE *f;
    char *mem;
    u32 len;
    hwrbitmap bg;

    f = fopen(s,"rb");
    if (f) {
      /* Read the bitmap from the file into a temporary buffer */
      fseek(f,0,SEEK_END);
      len = ftell(f);
      rewind(f);
      if (!iserror(g_malloc((void**)&mem,len))) {
	fread(mem,len,1,f);
	/* Tell the video engine to load the bitmap into memory
	 * in the current video mode's format.
	 */
	if (!iserror(VID(bitmap_load) (&bg,mem,len))) {
	  VID(blit) (vid->display,0,0,fbw,fbh,bg,0,0,PG_LGOP_NONE);
	  VID(bitmap_free) (bg);
	  sdlfb_update(NULL,0,0,fbw,fbh);
	}
	g_free(mem);
      }
      fclose(f);
    }
  }

  /* Offset vid->display to the specified position */
  sdlfb_display_x = get_param_int("video-sdlfb","display_x",0);
  sdlfb_display_y = get_param_int("video-sdlfb","display_y",0);
  FB_MEM += bpp * sdlfb_display_x / 8 + FB_BPL * sdlfb_display_y;

  /* tint and grayscale conversion */
#if defined(CONFIG_SDLEMU_COLOR) || defined(CONFIG_SDLEMU_BLIT)
  if (!sdlfb_emucolors) 
#endif
    {
    vid->color_pgtohwr = &sdlfb_tint_pgtohwr;
    vid->color_hwrtopg = &sdlfb_tint_hwrtopg;
  }

  /* Can't scale and use blit color emulation at the same time */
  if (sdlfb_backbuffer)
    sdlfb_scale = 1;
  
  /* If we're scaling, set up a backbuffer */
  if (sdlfb_scale != 1) {
    g_error e;
    FB_BPL = (vid->xres * vid->bpp) >> 3;
    e = g_malloc((void**)&FB_MEM,FB_BPL * vid->yres);    
    errorcheck;
    sdlfb_backbuffer = FB_MEM;
  }
  
#endif /* CONFIG_SDLSKIN */
   
  return success; 
}
   
void sdlfb_close(void) {
#if defined(CONFIG_SDLEMU_BLIT) || defined(CONFIG_SDLSKIN)
  /* Free backbuffer */
   if (sdlfb_backbuffer)
     g_free(sdlfb_backbuffer);
#endif   
  unload_inlib(inlib_main);   /* Take out our input driver */
  SDL_Quit();
}

void sdlfb_update(hwrbitmap d,s16 x,s16 y,s16 w,s16 h) {
  if (d!=vid->display)
    return;

  DBG("at %d,%d,%d,%d\n",x,y,w,h);

#ifdef CONFIG_SDLSKIN
   x += sdlfb_display_x;
   y += sdlfb_display_y;
#endif

#ifdef CONFIG_SDLSKIN
   if (sdlfb_scale != 1 && sdlfb_backbuffer) {
     u8 *src;
     u8 *dest;
     u8 *srcline;
     u8 *destline;
     int i,j,si,sj,pxw;
     
     /* Calculations */
     srcline = src = sdlfb_backbuffer + 
       (((x-sdlfb_display_x) * vid->bpp) >> 3) + (y-sdlfb_display_y)*FB_BPL;
     destline = dest = ((char*)sdl_vidsurf->pixels) +
       (((((x-sdlfb_display_x)*sdlfb_scale)+sdlfb_display_x)*vid->bpp)>>3) + 
       (((y-sdlfb_display_y)*sdlfb_scale)+sdlfb_display_y)*sdl_vidsurf->pitch;
     pxw = vid->bpp >> 3;

     /* Crufty little scale blit */
     for (j=h;j;j--,srcline+=FB_BPL)
       for (sj=sdlfb_scale;sj;sj--,dest=destline+=sdl_vidsurf->pitch) {
	 src = srcline;
	 for (i=w;i;i--,src+=pxw)
	   for (si=sdlfb_scale;si;si--,dest+=pxw)
	     memcpy(dest,src,pxw);
       }
     
     /* Munge coordinates */
     x = (x-sdlfb_display_x)*sdlfb_scale + sdlfb_display_x;
     y = (y-sdlfb_display_y)*sdlfb_scale + sdlfb_display_y;
     w *= sdlfb_scale;
     h *= sdlfb_scale;
   }
   else
#endif

   /* If we have both sdlemu_blit and skin support, scaling overrides
    * color emulation.
    */
   {
#ifdef CONFIG_SDLEMU_BLIT
     /* Do we need to convert and blit to the SDL buffer? */
     if (sdlfb_backbuffer) {
       u8 *src;
       u8 *dest;
       u8 *srcline;
       u8 *destline;
       int i,bw,j;
       int maxshift = 8 - vid->bpp;
       int shift;
       u8 c, mask = (1<<vid->bpp) - 1;
       
       /* Align it to a byte boundary (simplifies blit) */
       i = (8/vid->bpp) - 1;
       w += (x&i) + i;
       w &= ~i;
       x &= ~i;
       
       /* Calculations */
       srcline = src = sdlfb_backbuffer + ((x * vid->bpp) >> 3) +y*FB_BPL;
       destline = dest = (char*)sdl_vidsurf->pixels + x + y*sdl_vidsurf->pitch;
       bw = (w * vid->bpp) >> 3;
       
       /* Slow but it works (this is debug code, after all...) */
       for (j=h;j;j--,src=srcline+=FB_BPL,dest=destline+=sdl_vidsurf->pitch)
	 for (i=bw;i;i--,src++) {
#ifdef CONFIG_FB_PSION
	   /* Psion swaps pairs of pixels */
	   c = *src;
	   *(dest++) = c & mask;
	   *(dest++) = (c >> 4) & mask;
#else
           for (shift=maxshift,c=*src;shift>=0;shift-=vid->bpp)
	     *(dest++) = (c >> shift) & mask;
#endif
	 }
     }
#endif
   }

   /* Always let SDL update the front buffer */
#ifdef CONFIG_SDL_FULLUPDATE
   SDL_UpdateRect(sdl_vidsurf,0,0,vid->xres,vid->yres);
#else
   SDL_UpdateRect(sdl_vidsurf,x,y,w,h);
#endif
}

#ifdef CONFIG_SDLSKIN
/* Add an optional tint, for making LCDs look realistic */
pgcolor sdlfb_color_tint(pgcolor c) {
  u16 r,g,b;

  if (sdlfb_simbits) {
    int realbpp;
    realbpp = vid->bpp;
    vid->bpp = sdlfb_simbits;
    c = def_color_hwrtopg(def_color_pgtohwr(c));
    vid->bpp = realbpp;
  }

  if (sdlfb_tint != 0xFFFFFF) {
    /* Actual tinting */
    r  = getred(c);
    g  = getgreen(c);
    b  = getblue(c);
    r *= getred(sdlfb_tint);
    g *= getgreen(sdlfb_tint);
    b *= getblue(sdlfb_tint);
    r >>= 8;
    g >>= 8;
    b >>= 8;
    c = mkcolor(r,g,b);
  }

  return c;
}
pgcolor sdlfb_color_untint(pgcolor c) {
  u16 r,g,b;

  if (sdlfb_tint == 0xFFFFFF)
    return c;
  
  r  = getred(c) << 8;
  g  = getgreen(c) << 8;
  b  = getblue(c) << 8;
  r /= getred(sdlfb_tint);
  g /= getgreen(sdlfb_tint);
  b /= getblue(sdlfb_tint);

  /* FIXME: Hack! This forces colors close to white to be white, and
   * colors close to black to be black. This is just so things
   * won't look _too_ terrible when we turn the backlight on and
   * off a bunch.
   */
  if (r>200) r = 255;
  if (g>200) g = 255;
  if (b>200) b = 255;
  if (r<50) r = 0;
  if (g<50) g = 0;
  if (b<50) b = 0;

  return mkcolor(r,g,b);
}
hwrcolor sdlfb_tint_pgtohwr(pgcolor c) {
  if (c & PGCF_MASK) return def_color_pgtohwr(c);
  return def_color_pgtohwr(sdlfb_color_tint(c));
}
hwrcolor sdlfb_tint_hwrtopg(pgcolor c) {
  if (c & PGCF_MASK) return def_color_hwrtopg(c);
  return sdlfb_color_untint(def_color_hwrtopg(c));
}
#endif

#if defined(CONFIG_SDLEMU_COLOR) || defined(CONFIG_SDLEMU_BLIT)
hwrcolor sdlfbemu_color_pgtohwr(pgcolor c) {
#ifdef CONFIG_SDLSKIN
   c = sdlfb_color_tint(c);
#endif

   /* If this is black and white, be more conservative */
   if (sdlfb_emucolors==1)
     return (getred(c)+getgreen(c)+getblue(c))>0x80;
   else
     return (getred(c)+getgreen(c)+getblue(c))*sdlfb_emucolors/765;   
}

pgcolor sdlfbemu_color_hwrtopg(hwrcolor c) {
   u8 gray = c * 255/sdlfb_emucolors;
   return mkcolor(gray,gray,gray);
}
#endif

void sdlfb_message(u32 message, u32 param, u32 *ret) {
  switch (message) {

  case PGDM_SOUNDFX:
    /* This should be done using SDL's audio functions, this is
     * just a little kludge */
    {
      char beep[2] = "\a";
      write(1,beep,2);
    }
    break;

#ifdef CONFIG_SDLSKIN
  case PGDM_BACKLIGHT:
    /* Simulate the backlight by using an alternate tint color.
     * Go through the same procedure we use when changing color
     * depth in order to make everything re-convert their colors
     */
    bitmap_iterate((handle_iterator) vid->bitmap_modeunconvert, NULL);
    sdlfb_tint = strtol(get_param_str("video-sdlfb",
				      param ? "backlight_tint" : "tint",
				      "FFFFFF"),NULL,16);
    bitmap_iterate((handle_iterator) vid->bitmap_modeconvert, NULL);
    reload_initial_themes();
    break;
#endif

#ifdef CONFIG_SDLSDC
  case PGDM_SDC_CHAR:
    sdlfb_sdc_char( (char) param );
    break;
#endif

  }
}

#ifdef CONFIG_SDLSDC
g_error sdlfb_sdc_char(char c) {
  g_error e;
  struct font_descriptor *fd;
  struct font_metrics m;

  /* Don't bother if the SDC hasn't been configured */
  if (!(sdlsdc_w && sdlsdc_h))
    return success;

  /* If the font hasn't yet been allocated, set things up now */
  if (!sdlsdc_font) {
    struct font_style fs;

    memset(&fs,0,sizeof(fs));
    fs.name = get_param_str("video-sdlfb","sdc_font_name","");
    fs.size = get_param_int("video-sdlfb","sdc_font_size",0);
    fs.style = get_param_int("video-sdlfb","sdc_font_style",0);
    e = font_descriptor_create(&fd,&fs);
    errorcheck;
    e = mkhandle(&sdlsdc_font,PG_TYPE_FONTDESC,-1,fd);
    errorcheck;

    /* Convert colors to hwrcolor (ignore tint, simulated grays) */
    sdlsdc_hfg = def_color_pgtohwr(sdlsdc_fg);
    sdlsdc_hbg = def_color_pgtohwr(sdlsdc_bg);

    /* Clear the background */
    VID(rect) (&sdlsdc_bits, 0,0,sdlsdc_w,sdlsdc_h, sdlsdc_hbg, PG_LGOP_NONE);
    SDL_UpdateRect(sdl_vidsurf,sdlsdc_x,sdlsdc_y,sdlsdc_w,sdlsdc_h);
    
    /* Set the initial cursor position */
    fd->lib->getmetrics(fd,&m);
    sdlsdc_c.x = 0;
    sdlsdc_c.y = sdlsdc_h - m.lineheight;
  }
  
  /* Look up the font */
  e = rdhandle((void**) &fd, PG_TYPE_FONTDESC, -1, sdlsdc_font);
  errorcheck;
  fd->lib->getmetrics(fd,&m);

  /* If we got an explicit newline or the line is full,
   * scroll the display
   */
  if (c=='\n' || sdlsdc_c.x+m.charcell.w >= sdlsdc_w) {

    /* Reset cursor */
    sdlsdc_c.x = 0;

    /* Scroll */
    VID(blit) (&sdlsdc_bits,0,0,sdlsdc_w,sdlsdc_h-m.charcell.h,
	       &sdlsdc_bits,0,m.charcell.h,PG_LGOP_NONE);
    VID(rect) (&sdlsdc_bits,0,sdlsdc_h-m.charcell.h,
	       sdlsdc_w,m.charcell.h, sdlsdc_hbg, PG_LGOP_NONE);

    /* Update the whole thing */
    SDL_UpdateRect(sdl_vidsurf,sdlsdc_x,sdlsdc_y,sdlsdc_w,sdlsdc_h);
  }
  
  /* Handle backspace for overstriking- this will only look right for
   * fixed-width fonts */
  if (c == '\b') {
    sdlsdc_c.x -= m.charcell.w;
    if (sdlsdc_c.x < 0)
      sdlsdc_c.x = 0;
  }

  /* Plot a character */
  if (c!='\n' && c!='\r' && c!='\b') {
    fd->lib->draw_char(fd,&sdlsdc_bits,&sdlsdc_c,sdlsdc_hfg,c,NULL,PG_LGOP_NONE,0);
    
    /* Update the screen */
    SDL_UpdateRect(sdl_vidsurf,
		   sdlsdc_x+sdlsdc_c.x-m.charcell.w,
		   sdlsdc_y+sdlsdc_c.y,
		   m.charcell.w,
		   m.lineheight);
  }

  return success;
}
#endif

g_error sdlfb_regfunc(struct vidlib *v) {
  v->init = &sdlfb_init;
  v->setmode = &sdlfb_setmode; 
  v->close = &sdlfb_close;
  v->update = &sdlfb_update;    
  v->message = &sdlfb_message;
  return success;
}

/* The End */









