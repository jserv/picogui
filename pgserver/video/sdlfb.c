/* $Id: sdlfb.c,v 1.18 2001/08/12 21:08:58 micahjd Exp $
 *
 * sdlfb.c - This driver provides an interface between the linear VBLs
 *           and a framebuffer provided by the SDL graphics library.
 *           It can run on many output devices, but SDL is most often used
 *           with the X window system. This SDL driver has facilities for
 *           simulating embedded platforms: low-bpp emulation and Skins.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>

#include <pgserver/video.h>
#include <pgserver/input.h>

#include <SDL.h>

#ifdef CONFIG_SDLSKIN
#include <stdio.h>        /* File I/O for loading skin bitmap */
#endif

SDL_Surface *sdl_vidsurf;
#if defined(CONFIG_SDLEMU_COLOR) || defined(CONFIG_SDLEMU_BLIT)
int sdlfb_emucolors;
#endif

#ifdef CONFIG_SDLSKIN
pgcolor sdlfb_tint;
s16 sdlfb_display_x;
s16 sdlfb_display_y;
#endif

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

hwrcolor sdlfbemu_color_pgtohwr(pgcolor c);
pgcolor sdlfbemu_color_hwrtopg(hwrcolor c);
g_error sdlfb_init(void);
void sdlfb_close(void);
void sdlfb_update(s16 x,s16 y,s16 w,s16 h);
g_error sdlfb_regfunc(struct vidlib *v);
g_error sdlfb_setmode(s16 xres,s16 yres,s16 bpp,u32 flags);
hwrcolor sdlfb_tint_pgtohwr(pgcolor c);
pgcolor sdlfb_color_tint(pgcolor c);

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
  unsigned long sdlflags = SDL_RESIZABLE;
  char str[80];
  SDL_Color palette[256];
  int i;
  s16 fbw,fbh;
#ifdef CONFIG_SDLSKIN
  char *s;
#endif

#ifdef CONFIG_SDLEMU_BLIT
   /* Make screen divisible by a byte */
  if (bpp && bpp<8)
     xres &= ~((8/bpp)-1);
#endif
  
  fbw = xres;
  fbh = yres;

  /* Interpret flags */
  if (get_param_int("video-sdlfb","fullscreen",0))
    sdlflags |= SDL_FULLSCREEN;
#ifdef CONFIG_SDLSKIN
  if (i = get_param_int("video-sdlfb","width",0)) {
    fbw = i;
    sdlflags &= ~SDL_RESIZABLE;
  }
  if (i = get_param_int("video-sdlfb","height",0)) {
    fbh = i;
    sdlflags &= ~SDL_RESIZABLE;
  }
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
     /* If this is 8bpp set up a 2-3-3 palette for pseudo-RGB */
#endif
     {
	for (i=0;i<256;i++) {
	   palette[i].r = (i & 0xC0) * 255 / 0xC0;
	   palette[i].g = (i & 0x38) * 255 / 0x38;
	   palette[i].b = (i & 0x07) * 255 / 0x07;
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
     FB_BPL = (vid->xres * bpp) >> 3;
     /* The +1 allows blits some margin to read unnecessary bytes.
      * Keeps linear1's blit from triggering electric fence when the
      * cursor is put in the bottom-right corner ;-)
      */
     e = g_malloc((void**)&FB_MEM,(FB_BPL * vid->yres) + 1);
     errorcheck;
  }
  else
#endif
  {
     vid->bpp  = sdl_vidsurf->format->BitsPerPixel;
     FB_MEM = sdl_vidsurf->pixels;
     FB_BPL = sdl_vidsurf->pitch;
  }
   
  /* Info */
  sprintf(str,get_param_str("video-sdlfb","caption","PicoGUI (sdl@%dx%dx%d)"),
	  vid->xres,vid->yres,bpp);
  SDL_WM_SetCaption(str,NULL);

#ifdef CONFIG_SDLSKIN
  /* Offset vid->display to the specified position */
  sdlfb_display_x = get_param_int("video-sdlfb","display_x",0);
  sdlfb_display_y = get_param_int("video-sdlfb","display_y",0);
  FB_MEM += bpp * sdlfb_display_x / 8;
  FB_MEM += FB_BPL * sdlfb_display_y;

  /* Install the skin background */
  if (s = get_param_str("video-sdlfb","background",NULL)) {
    FILE *f;
    char *mem;
    unsigned long len;
    hwrbitmap bg;
    struct stdbitmap dest;

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
	  /* Construct a destination bitmap for the raw SDL
	   * framebuffer (vid->display is just the section that the
	   * rest of PicoGUI can use) and blit the background. It
	   * will tile if the background is smaller than the framebuffer.
	   */
	  memset(&dest,0,sizeof(dest));
	  dest.bits  = sdl_vidsurf->pixels;
	  dest.w     = fbw;
	  dest.h     = fbh;
	  dest.pitch = sdl_vidsurf->pitch;
	  VID(blit) (&dest,0,0,fbw,fbh,bg,0,0,PG_LGOP_NONE);
	  VID(bitmap_free) (bg);
	}
	g_free(mem);
      }
      fclose(f);
    }
  }

  /* Load initial tint */
  sdlfb_tint = strtol(get_param_str("video-sdlfb","tint","FFFFFF"),NULL,16);
  if (sdlfb_tint != 0xFFFFFF)
    vid->color_pgtohwr = &sdlfb_tint_pgtohwr;
#endif
   
  return sucess; 
}
   
void sdlfb_close(void) {
#ifdef CONFIG_SDLEMU_BLIT
  /* Free backbuffer */
   if (FB_MEM && (FB_MEM != sdl_vidsurf->pixels))
     g_free(FB_MEM);
#endif   
  unload_inlib(inlib_main);   /* Take out our input driver */
  SDL_Quit();
}

void sdlfb_update(s16 x,s16 y,s16 w,s16 h) {
#ifdef CONFIG_SDLSKIN
  x += sdlfb_display_x;
  y += sdlfb_display_y;
#endif

#ifdef DEBUG_VIDEO
   printf("sdlfb_update(%d,%d,%d,%d)\n",x,y,w,h);
#endif

#ifdef CONFIG_SDLEMU_BLIT
   /* Do we need to convert and blit to the SDL buffer? */
   if (FB_MEM != sdl_vidsurf->pixels) {
      unsigned char *src;
      unsigned char *dest;
      unsigned char *srcline;
      unsigned char *destline;
      int i,bw,j;
      int maxshift = 8 - vid->bpp;
      int shift;
      unsigned char c, mask = (1<<vid->bpp) - 1;

      /* Align it to an 8-pixel boundary (simplifies blit) */
      w += (x&7) + 7;
      w &= ~7;
      x &= ~7;
      
      /* Calculations */
      srcline = src = FB_MEM + ((x * vid->bpp) >> 3) +y*FB_BPL;
      destline = dest = sdl_vidsurf->pixels + x + y*vid->xres;
      bw = (w * vid->bpp) >> 3;
      
      /* Slow but it works (this is debug code, after all...) */
      for (j=h;j;j--,src=srcline+=FB_BPL,dest=destline+=sdl_vidsurf->pitch)
	for (i=bw;i;i--,src++)
	  for (shift=maxshift,c=*src;shift>=0;shift-=vid->bpp)
	    *(dest++) = (c >> shift) & mask;
   }
#endif

   /* Always let SDL update the front buffer */
   SDL_UpdateRect(sdl_vidsurf,x,y,w,h);
}

#ifdef CONFIG_SDLSKIN
/* Add an optional tint, for making LCDs look realistic */
pgcolor sdlfb_color_tint(pgcolor c) {
  u16 r,g,b;

  if (sdlfb_tint == 0xFFFFFF)
    return c;
  
  r  = getred(c);
  g  = getgreen(c);
  b  = getblue(c);
  r *= getred(sdlfb_tint);
  g *= getgreen(sdlfb_tint);
  b *= getblue(sdlfb_tint);

  return mkcolor(r>>8,g>>8,b>>8);
}
hwrcolor sdlfb_tint_pgtohwr(pgcolor c) {
  return def_color_pgtohwr(sdlfb_color_tint(c));
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
   unsigned char gray = c * 255/sdlfb_emucolors;
   return mkcolor(gray,gray,gray);
}
#endif

void sdlfb_message(u32 message, u32 param) {
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
    /* Simulate the backlight by using an alternate tint color */
    sdlfb_tint = strtol(get_param_str("video-sdlfb",
				      param ? "backlight_tint" : "tint",
				      "FFFFFF"),NULL,16);
    break;
#endif

  }
}

g_error sdlfb_regfunc(struct vidlib *v) {
  v->init = &sdlfb_init;
  v->setmode = &sdlfb_setmode; 
  v->close = &sdlfb_close;
  v->update = &sdlfb_update;    
  v->message = &sdlfb_message;
  return sucess;
}

/* The End */
