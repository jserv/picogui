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
   
#ifdef DRIVER_SVGAFB      
  DRV("svgafb",&svgafb_regfunc)
#endif

#ifdef DRIVER_SVGAGL
  DRV("svgagl",&svgagl_regfunc)
#endif   

#ifdef DRIVER_EZ328
  DRV("ez328",&ez328_regfunc)
#endif

#ifdef DRIVER_NCURSES
  DRV("ncurses",&ncurses_regfunc)
#endif   

#ifdef DRIVER_SERIAL40X4
  DRV("serial40x4",&serial40x4_regfunc)
#endif   

#ifdef DRIVER_NULL
  DRV("null",&null_regfunc)
#endif DRIVER_NULL
   
/* The End */
