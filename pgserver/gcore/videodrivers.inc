/* Video 
 * Order does matter here- if no driver is specified, the one listed 
 * first here will be tried first, trying the next if there is an error,
 * and so on...
 *
 * (see driverinfo.c for more info)
 */

#ifdef DRIVER_SDLACCEL
  DRV("sdlaccel",&sdlaccel_regfunc)
#endif

#ifdef DRIVER_FBDEV
  DRV("fbdev",&fbdev_regfunc)
#endif
   
#ifdef DRIVER_SDLFB
  DRV("sdlfb",&sdlfb_regfunc)
#endif
   
#ifdef DRIVER_SVGAFB      
  DRV("svgafb",&svgafb_regfunc)
#endif

#ifdef DRIVER_SVGAGL
  DRV("svgagl",&svgagl_regfunc)
#endif   

#ifdef DRIVER_EZ328
  DRV("ez328",&ez328_regfunc)
#endif

#ifdef DRIVER_EZ328VGA
  DRV("ez328vga",&ez328vga_regfunc)
#endif

#ifdef DRIVER_NCURSES
  DRV("ncurses",&ncurses_regfunc)
#endif   

#ifdef DRIVER_SERIAL40X4
  DRV("serial40x4",&serial40x4_regfunc)
#endif   

#ifdef DRIVER_SCRSHOT
  DRV("scrshot",&scrshot_regfunc)
#endif

#ifdef DRIVER_NULLFB
  DRV("nullfb",&nullfb_regfunc)
#endif DRIVER_NULLFB

#ifdef DRIVER_NULL
  DRV("null",&null_regfunc)
#endif DRIVER_NULL

#ifdef DRIVER_S1D13806
  DRV("s1d13806",&s1d13806_regfunc)
#endif DRIVER_S1D13806
   
/* The End */
