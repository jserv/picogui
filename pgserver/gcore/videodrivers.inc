/* Video 
 * Order does matter here- if no driver is specified, the one listed 
 * first here will be tried first, trying the next if there is an error,
 * and so on...
 *
 * (see driverinfo.c for more info)
 */

#ifdef DRIVER_FBDEV
  DRV("fbdev",&fbdev_regfunc)
#endif
   
#ifdef DRIVER_SDLFB
  DRV("sdlfb",&sdlfb_regfunc)
#endif
   
#ifdef DRIVER_SDL
  DRV("sdl",&sdl_regfunc)
#endif
   
#ifdef DRIVER_SVGAFB      
  DRV("svgafb",&svgafb_regfunc)
#endif

#ifdef DRIVER_SVGAGL
  DRV("svgagl",&svgagl_regfunc)
#endif   

#ifdef DRIVER_EZ328_CHIPSLICE
#  ifdef DRIVER_EZ328_CHIPSLICE_V0_2_CITIZEN_G3243H
     DRV("ez328_chipslice_vd2_citizen_G3243H",&chipslice_video_regfunc)
#  endif
#endif

#ifdef DRIVER_EZ328
  DRV("ez328",&ez328_regfunc)
#endif

#ifdef DRIVER_NCURSES
  DRV("ncurses",&ncurses_regfunc)
#endif   

#ifdef DRIVER_NULL
  DRV("null",&null_regfunc)
#endif DRIVER_NULL
   
/* The End */
