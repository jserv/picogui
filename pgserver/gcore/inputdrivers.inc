/* Input
 * Usually this will be autoloaded by the video driver.  If no driver is
 * specified, no input. Order does not matter
 *
 * (see driverinfo.c for more info)
 */

#ifdef DRIVER_SDLINPUT
  DRV("sdlinput",&sdlinput_regfunc)
#endif
   
#ifdef DRIVER_SVGAINPUT
  DRV("svgainput",&svgainput_regfunc)
#endif

#ifdef DRIVER_NCURSESINPUT
  DRV("ncursesinput",&ncursesinput_regfunc)
#endif

#ifdef DRIVER_TSINPUT
  DRV("tsinput",&tsinput_regfunc)
#endif
   
/* The End */
